#include <GL/glut.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <string>
#include <sstream>
#include <fstream>
#include <algorithm>

using namespace std;

// Estructuras de datos
struct Point {
    int x, y;
    Point(int x = 0, int y = 0) : x(x), y(y) {}
};

struct Color {
    float r, g, b;
    Color(float r = 0.0f, float g = 0.0f, float b = 0.0f) : r(r), g(g), b(b) {}
};

struct Figure {
    int type; // 0: línea directa, 1: línea DDA, 2: círculo incremental, 3: círculo punto medio, 4: elipse
    vector<Point> points;
    Color color;
    int thickness;
};

// Variables globales
const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;
const int GRID_SIZE = 20;

vector<Figure> figures;
vector<Figure> redoStack;
vector<Point> currentPoints;
Color currentColor(0.0f, 0.0f, 0.0f);
int currentThickness = 1;
int currentTool = 0; // 0: línea directa, 1: línea DDA, 2: círculo incremental, 3: círculo punto medio, 4: elipse
bool showGrid = true;
bool showAxes = true;
bool showCoords = false;
bool drawing = false;
bool needsRedisplay = false;

// Prototipos de funciones
void drawPixel(int x, int y, Color color, int thickness = 1);
void drawLineDirect(Point p1, Point p2, Color color, int thickness);
void drawLineDDA(Point p1, Point p2, Color color, int thickness);
void drawCircleIncremental(Point center, int radius, Color color, int thickness);
void drawCircleMidpoint(Point center, int radius, Color color, int thickness);
void drawEllipseMidpoint(Point center, int rx, int ry, Color color, int thickness);
void drawGrid();
void drawAxes();
void displayCoordinates(int x, int y);
void exportToPPM(const string& filename);

// Implementación de algoritmos de rasterización
void drawPixel(int x, int y, Color color, int thickness) {
    glColor3f(color.r, color.g, color.b);
    glPointSize(thickness);
    glBegin(GL_POINTS);
    glVertex2i(x, y);
    glEnd();
}

void drawLineDirect(Point p1, Point p2, Color color, int thickness) {
    int dx = p2.x - p1.x;
    int dy = p2.y - p1.y;

    if (dx == 0) { // Línea vertical
        int yStart = min(p1.y, p2.y);
        int yEnd = max(p1.y, p2.y);
        for (int y = yStart; y <= yEnd; y++) {
            drawPixel(p1.x, y, color, thickness);
        }
        return;
    }

    float m = static_cast<float>(dy) / dx;
    float b = p1.y - m * p1.x;

    if (abs(m) <= 1.0f) {
        int xStart = min(p1.x, p2.x);
        int xEnd = max(p1.x, p2.x);
        for (int x = xStart; x <= xEnd; x++) {
            int y = round(m * x + b);
            drawPixel(x, y, color, thickness);
        }
    } else {
        int yStart = min(p1.y, p2.y);
        int yEnd = max(p1.y, p2.y);
        for (int y = yStart; y <= yEnd; y++) {
            int x = round((y - b) / m);
            drawPixel(x, y, color, thickness);
        }
    }
}

void drawLineDDA(Point p1, Point p2, Color color, int thickness) {
    int dx = p2.x - p1.x;
    int dy = p2.y - p1.y;
    int steps = max(abs(dx), abs(dy));

    if (steps == 0) {
        drawPixel(p1.x, p1.y, color, thickness);
        return;
    }

    float xIncrement = static_cast<float>(dx) / steps;
    float yIncrement = static_cast<float>(dy) / steps;

    float x = p1.x;
    float y = p1.y;

    for (int i = 0; i <= steps; i++) {
        drawPixel(round(x), round(y), color, thickness);
        x += xIncrement;
        y += yIncrement;
    }
}

void drawCircleIncremental(Point center, int radius, Color color, int thickness) {
    float angle = 0;
    float angleIncrement = 1.0f / radius;

    while (angle < 2 * M_PI) {
        int x = center.x + radius * cos(angle);
        int y = center.y + radius * sin(angle);
        drawPixel(x, y, color, thickness);
        angle += angleIncrement;
    }
}

void drawCircleMidpoint(Point center, int radius, Color color, int thickness) {
    int x = 0;
    int y = radius;
    int d = 1 - radius;

    auto drawCirclePoints = [&](int x, int y) {
        drawPixel(center.x + x, center.y + y, color, thickness);
        drawPixel(center.x - x, center.y + y, color, thickness);
        drawPixel(center.x + x, center.y - y, color, thickness);
        drawPixel(center.x - x, center.y - y, color, thickness);
        drawPixel(center.x + y, center.y + x, color, thickness);
        drawPixel(center.x - y, center.y + x, color, thickness);
        drawPixel(center.x + y, center.y - x, color, thickness);
        drawPixel(center.x - y, center.y - x, color, thickness);
    };

    drawCirclePoints(x, y);

    while (y > x) {
        x++;
        if (d < 0) {
            d += 2 * x + 1;
        } else {
            y--;
            d += 2 * (x - y) + 1;
        }
        drawCirclePoints(x, y);
    }
}

void drawEllipseMidpoint(Point center, int rx, int ry, Color color, int thickness) {
    if (rx <= 0 || ry <= 0) return;

    int rx2 = rx * rx;
    int ry2 = ry * ry;
    int twoRx2 = 2 * rx2;
    int twoRy2 = 2 * ry2;

    int x = 0;
    int y = ry;
    int px = 0;
    int py = twoRx2 * y;

    // Región 1
    int p = round(ry2 - (rx2 * ry) + (0.25 * rx2));
    while (px < py) {
        x++;
        px += twoRy2;
        if (p < 0) {
            p += ry2 + px;
        } else {
            y--;
            py -= twoRx2;
            p += ry2 + px - py;
        }

        drawPixel(center.x + x, center.y + y, color, thickness);
        drawPixel(center.x - x, center.y + y, color, thickness);
        drawPixel(center.x + x, center.y - y, color, thickness);
        drawPixel(center.x - x, center.y - y, color, thickness);
    }

    // Región 2
    p = round(ry2 * (x + 0.5) * (x + 0.5) + rx2 * (y - 1) * (y - 1) - rx2 * ry2);
    while (y > 0) {
        y--;
        py -= twoRx2;
        if (p > 0) {
            p += rx2 - py;
        } else {
            x++;
            px += twoRy2;
            p += rx2 - py + px;
        }

        drawPixel(center.x + x, center.y + y, color, thickness);
        drawPixel(center.x - x, center.y + y, color, thickness);
        drawPixel(center.x + x, center.y - y, color, thickness);
        drawPixel(center.x - x, center.y - y, color, thickness);
    }
}

// Funciones de dibujo auxiliares
void drawGrid() {
    glColor3f(0.8f, 0.8f, 0.8f);
    glBegin(GL_LINES);

    // Líneas verticales
    for (int x = -WINDOW_WIDTH/2; x <= WINDOW_WIDTH/2; x += GRID_SIZE) {
        glVertex2i(x, -WINDOW_HEIGHT/2);
        glVertex2i(x, WINDOW_HEIGHT/2);
    }

    // Líneas horizontales
    for (int y = -WINDOW_HEIGHT/2; y <= WINDOW_HEIGHT/2; y += GRID_SIZE) {
        glVertex2i(-WINDOW_WIDTH/2, y);
        glVertex2i(WINDOW_WIDTH/2, y);
    }

    glEnd();
}

void drawAxes() {
    glColor3f(0.5f, 0.5f, 0.5f);
    glBegin(GL_LINES);

    // Eje X
    glVertex2i(-WINDOW_WIDTH/2, 0);
    glVertex2i(WINDOW_WIDTH/2, 0);

    // Eje Y
    glVertex2i(0, -WINDOW_HEIGHT/2);
    glVertex2i(0, WINDOW_HEIGHT/2);

    glEnd();
}

void displayCoordinates(int x, int y) {
    // Convertir coordenadas de pantalla a coordenadas del mundo
    int worldX = x - WINDOW_WIDTH/2;
    int worldY = WINDOW_HEIGHT/2 - y;

    // Dibujar texto con las coordenadas
    glColor3f(1.0f, 1.0f, 1.0f);
    glRasterPos2i(10, 10);

    stringstream ss;
    ss << "(" << worldX << ", " << worldY << ")";
    string coordStr = ss.str();

    for (char c : coordStr) {
        glutBitmapCharacter(GLUT_BITMAP_9_BY_15, c);
    }
}

void exportToPPM(const string& filename) {
    ofstream file(filename, ios::binary);
    if (!file) {
        cerr << "Error al crear el archivo: " << filename << endl;
        return;
    }

    // Encabezado PPM
    file << "P6\n" << WINDOW_WIDTH << " " << WINDOW_HEIGHT << "\n255\n";

    // Capturar el buffer de píxeles
    vector<unsigned char> pixels(3 * WINDOW_WIDTH * WINDOW_HEIGHT);
    glReadPixels(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());

    // Escribir los píxeles (invertir verticalmente)
    for (int y = WINDOW_HEIGHT - 1; y >= 0; --y) {
        file.write(reinterpret_cast<const char*>(&pixels[y * WINDOW_WIDTH * 3]), WINDOW_WIDTH * 3);
    }

    file.close();
    cout << "Imagen exportada como: " << filename << endl;
}

// Callbacks de OpenGL/GLUT
void display() {
    glClear(GL_COLOR_BUFFER_BIT);

    if (showGrid) drawGrid();
    if (showAxes) drawAxes();

    // Dibujar todas las figuras
    for (const auto& figure : figures) {
        switch (figure.type) {
            case 0: // Línea directa
                if (figure.points.size() >= 2)
                    drawLineDirect(figure.points[0], figure.points[1], figure.color, figure.thickness);
                break;
            case 1: // Línea DDA
                if (figure.points.size() >= 2)
                    drawLineDDA(figure.points[0], figure.points[1], figure.color, figure.thickness);
                break;
            case 2: // Círculo incremental
                if (figure.points.size() >= 2) {
                    int radius = static_cast<int>(sqrt(
                        pow(figure.points[1].x - figure.points[0].x, 2) +
                        pow(figure.points[1].y - figure.points[0].y, 2)
                    ));
                    drawCircleIncremental(figure.points[0], radius, figure.color, figure.thickness);
                }
                break;
            case 3: // Círculo punto medio
                if (figure.points.size() >= 2) {
                    int radius = static_cast<int>(sqrt(
                        pow(figure.points[1].x - figure.points[0].x, 2) +
                        pow(figure.points[1].y - figure.points[0].y, 2)
                    ));
                    drawCircleMidpoint(figure.points[0], radius, figure.color, figure.thickness);
                }
                break;
            case 4: // Elipse
                if (figure.points.size() >= 2) {
                    int rx = abs(figure.points[1].x - figure.points[0].x);
                    int ry = abs(figure.points[1].y - figure.points[0].y);
                    drawEllipseMidpoint(figure.points[0], rx, ry, figure.color, figure.thickness);
                }
                break;
        }
    }

    // Dibujar figura actual en proceso
    if (drawing && currentPoints.size() > 0) {
        switch (currentTool) {
            case 0: // Línea directa
                if (currentPoints.size() >= 2)
                    drawLineDirect(currentPoints[0], currentPoints[1], currentColor, currentThickness);
                break;
            case 1: // Línea DDA
                if (currentPoints.size() >= 2)
                    drawLineDDA(currentPoints[0], currentPoints[1], currentColor, currentThickness);
                break;
            case 2: // Círculo incremental
                if (currentPoints.size() >= 2) {
                    int radius = static_cast<int>(sqrt(
                        pow(currentPoints[1].x - currentPoints[0].x, 2) +
                        pow(currentPoints[1].y - currentPoints[0].y, 2)
                    ));
                    drawCircleIncremental(currentPoints[0], radius, currentColor, currentThickness);
                }
                break;
            case 3: // Círculo punto medio
                if (currentPoints.size() >= 2) {
                    int radius = static_cast<int>(sqrt(
                        pow(currentPoints[1].x - currentPoints[0].x, 2) +
                        pow(currentPoints[1].y - currentPoints[0].y, 2)
                    ));
                    drawCircleMidpoint(currentPoints[0], radius, currentColor, currentThickness);
                }
                break;
            case 4: // Elipse
                if (currentPoints.size() >= 2) {
                    int rx = abs(currentPoints[1].x - currentPoints[0].x);
                    int ry = abs(currentPoints[1].y - currentPoints[0].y);
                    drawEllipseMidpoint(currentPoints[0], rx, ry, currentColor, currentThickness);
                }
                break;
        }
    }

    if (showCoords) {
        int mouseX = glutGet(GLUT_WINDOW_X) + glutGet(GLUT_WINDOW_WIDTH) / 2;
        int mouseY = glutGet(GLUT_WINDOW_Y) + glutGet(GLUT_WINDOW_HEIGHT) / 2;
        displayCoordinates(mouseX, mouseY);
    }

    glutSwapBuffers();
}

void mouse(int button, int state, int x, int y) {
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        // Convertir coordenadas de pantalla a coordenadas del mundo
        int worldX = x - WINDOW_WIDTH/2;
        int worldY = WINDOW_HEIGHT/2 - y;

        if (!drawing) {
            drawing = true;
            currentPoints.clear();
        }

        currentPoints.push_back(Point(worldX, worldY));

        // Si tenemos los puntos necesarios, finalizar el dibujo
        if ((currentTool <= 1 && currentPoints.size() == 2) || // Líneas
            (currentTool >= 2 && currentPoints.size() == 2)) { // Círculos y elipses
            Figure newFigure;
            newFigure.type = currentTool;
            newFigure.points = currentPoints;
            newFigure.color = currentColor;
            newFigure.thickness = currentThickness;

            figures.push_back(newFigure);
            redoStack.clear(); // Limpiar pila de rehacer al hacer una nueva acción

            drawing = false;
            currentPoints.clear();
        }

        glutPostRedisplay();
    }
}

void keyboard(unsigned char key, int x, int y) {
    switch (key) {
        case 'g':
        case 'G':
            showGrid = !showGrid;
            break;
        case 'e':
        case 'E':
            showAxes = !showAxes;
            break;
        case 'c':
        case 'C':
            figures.clear();
            redoStack.clear();
            break;
        case 's':
        case 'S':
            exportToPPM("output.ppm");
            break;
        case 'z':
        case 'Z':
            if (!figures.empty()) {
                redoStack.push_back(figures.back());
                figures.pop_back();
            }
            break;
        case 'y':
        case 'Y':
            if (!redoStack.empty()) {
                figures.push_back(redoStack.back());
                redoStack.pop_back();
            }
            break;
    }
    glutPostRedisplay();
}

void menu(int value) {
    switch (value) {
        // Herramientas de dibujo
        case 0: currentTool = 0; break; // Línea directa
        case 1: currentTool = 1; break; // Línea DDA
        case 2: currentTool = 2; break; // Círculo incremental
        case 3: currentTool = 3; break; // Círculo punto medio
        case 4: currentTool = 4; break; // Elipse punto medio

        // Colores
        case 10: currentColor = Color(0.0f, 0.0f, 0.0f); break; // Negro
        case 11: currentColor = Color(1.0f, 0.0f, 0.0f); break; // Rojo
        case 12: currentColor = Color(0.0f, 1.0f, 0.0f); break; // Verde
        case 13: currentColor = Color(0.0f, 0.0f, 1.0f); break; // Azul
        case 14:
            // Personalizado (aquí se podría implementar un selector de color)
            currentColor = Color(1.0f, 1.0f, 0.0f); // Amarillo por defecto
            break;

        // Grosor
        case 20: currentThickness = 1; break;
        case 21: currentThickness = 2; break;
        case 22: currentThickness = 3; break;
        case 23: currentThickness = 5; break;

        // Vista
        case 30: showGrid = !showGrid; break;
        case 31: showAxes = !showAxes; break;
        case 32: showCoords = !showCoords; break;

        // Herramientas
        case 40: figures.clear(); redoStack.clear(); break; // Limpiar lienzo
        case 41:
            if (!figures.empty()) {
                redoStack.push_back(figures.back());
                figures.pop_back();
            }
            break; // Deshacer
        case 42: exportToPPM("output.ppm"); break; // Exportar

        // Ayuda
        case 50:
            cout << "Atajos de teclado:" << endl;
            cout << "G: Mostrar/ocultar cuadrícula" << endl;
            cout << "E: Mostrar/ocultar ejes" << endl;
            cout << "C: Limpiar lienzo" << endl;
            cout << "S: Exportar imagen" << endl;
            cout << "Z: Deshacer" << endl;
            cout << "Y: Rehacer" << endl;
            break;
        case 51:
            cout << "Software CAD 2D Básico" << endl;
            cout << "Implementado con OpenGL/FreeGLUT" << endl;
            cout << "2025" << endl;
            break;
    }
    glutPostRedisplay();
}

void createMenu() {
    int drawMenu = glutCreateMenu(menu);
    glutAddMenuEntry("Línea (Método Directo)", 0);
    glutAddMenuEntry("Línea (DDA)", 1);
    glutAddMenuEntry("Círculo (Incremental)", 2);
    glutAddMenuEntry("Círculo (Punto Medio)", 3);
    glutAddMenuEntry("Elipse (Punto Medio)", 4);

    int colorMenu = glutCreateMenu(menu);
    glutAddMenuEntry("Negro", 10);
    glutAddMenuEntry("Rojo", 11);
    glutAddMenuEntry("Verde", 12);
    glutAddMenuEntry("Azul", 13);
    glutAddMenuEntry("Personalizado", 14);

    int thicknessMenu = glutCreateMenu(menu);
    glutAddMenuEntry("1 px", 20);
    glutAddMenuEntry("2 px", 21);
    glutAddMenuEntry("3 px", 22);
    glutAddMenuEntry("5 px", 23);

    int viewMenu = glutCreateMenu(menu);
    glutAddMenuEntry("Mostrar/Ocultar Cuadrícula", 30);
    glutAddMenuEntry("Mostrar/Ocultar Ejes", 31);
    glutAddMenuEntry("Mostrar Coordenadas", 32);

    int toolsMenu = glutCreateMenu(menu);
    glutAddMenuEntry("Limpiar Lienzo", 40);
    glutAddMenuEntry("Deshacer", 41);
    glutAddMenuEntry("Exportar Imagen", 42);

    int helpMenu = glutCreateMenu(menu);
    glutAddMenuEntry("Atajos de Teclado", 50);
    glutAddMenuEntry("Acerca de", 51);

    int mainMenu = glutCreateMenu(menu);
    glutAddSubMenu("Dibujo", drawMenu);
    glutAddSubMenu("Color", colorMenu);
    glutAddSubMenu("Grosor", thicknessMenu);
    glutAddSubMenu("Vista", viewMenu);
    glutAddSubMenu("Herramientas", toolsMenu);
    glutAddSubMenu("Ayuda", helpMenu);

    glutAttachMenu(GLUT_RIGHT_BUTTON);
}

void init() {
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(-WINDOW_WIDTH/2, WINDOW_WIDTH/2, -WINDOW_HEIGHT/2, WINDOW_HEIGHT/2);
    glMatrixMode(GL_MODELVIEW);
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("Software CAD 2D Básico");

    init();
    createMenu();

    glutDisplayFunc(display);
    glutMouseFunc(mouse);
    glutKeyboardFunc(keyboard);

    cout << "Software CAD 2D Básico" << endl;
    cout << "Use el botón derecho para acceder al menú" << endl;
    cout << "Atajos: G (cuadrícula), E (ejes), C (limpiar), S (exportar), Z (deshacer), Y (rehacer)" << endl;

    glutMainLoop();
    return 0;
}
