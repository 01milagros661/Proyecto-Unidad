#include <GL/glut.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <string>
#include <sstream>
#include <fstream>

using namespace std;


struct Point {
    int x, y;
    Point(int x = 0, int y = 0) : x(x), y(y) {}
};

struct Figure {
    int type;
    vector<Point> points;
    float color[3];
    int thickness;
};


int windowWidth = 800;
int windowHeight = 600;
bool showGrid = true;
bool showAxes = true;
bool showCoords = false;
bool showHelp = false;
bool showAbout = false;
int gridSpacing = 20;
float currentColor[3] = {0.0, 0.0, 0.0};
int currentThickness = 1;
int drawingMode = -1;
vector<Point> currentPoints;
vector<Figure> figures;
vector<vector<Figure>> history;
int historyIndex = -1;
