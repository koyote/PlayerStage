#include "PathPlanner.h"

/*
 * Returns false if the location is out of bounds or an obstacle.
 */
bool Astar::isValidLocation(int sx, int sy, int x, int y) {
    bool invalid = (x < 0) || (y < 0) || (x >= MAPSIZE_X) || (y >= MAPSIZE_Y);

    if ((!invalid) && ((sx != x) || (sy != y))) {
        invalid = lr->isObst(x, y);
    }

    return !invalid;
}

/*
 * Returns the cost of moving.
 */
double getMovCost(int currX, int currY, int x, int y) {
    // if (currX != x && currY != y) {
    //    return 1.4;
    //} else {
        return 1.0;
    //}
}

/*
 * Returns an estimate of the distance between (x,y) and (tx,ty).
 */
float getHeuCost(int x, int y, int tx, int ty) {
    float dx = tx - x;
    float dy = ty - y;

    return sqrt((dx * dx)+(dy * dy));
    // return hypot(tx - x, ty - y);
}

/*
 * Returns true if the Node is in the specified list.
 */
bool inList(list<Node> &list, Node & node) {
    std::list<Node>::iterator i;
    for (i = list.begin(); i != list.end(); ++i) {
        if (i->x == node.x && i->y == node.y) {
            return true;
        }
    }
    return false;
}

/*
 * Overloading for finding the closest point (converts the coordinates to
 * matrix values).
 */
player_pose2d_t Astar::findClosest(double currX, double currY) {
    return findClosest(lr->getMatrixValue(currX), lr->getMatrixValue(currY));
}

/*
 * Returns the closest unseen, accessible, point to the robot's current position.
 */
player_pose2d_t Astar::findClosest(int x, int y) {
    Path *mainPath = new Path(-1, -1, 1000000); // If no path found, returned path will be smaller than the XY_BOUND
    for (int i = 0; i < MAPSIZE_X; i++) {
        for (int j = 0; j < MAPSIZE_Y; j++) {
            if (lr->getCoorValue(i) <= -(X_BOUND - 0.2) || lr->getCoorValue(i) >= (X_BOUND - 0.2) || lr->getCoorValue(j) <= -(Y_BOUND - 0.2) || lr->getCoorValue(j) >= (Y_BOUND - 0.2)) { //hack
                continue;
            }
            if (!lr->isSeen(i, j) && i != x && j != y && !lr->isObst(i, j)) {
                double currentHeu = getHeuCost(x, y, i, j);
                if (currentHeu < mainPath->cost) {
                    mainPath->x = i;
                    mainPath->y = j;
                    mainPath->cost = currentHeu;
                }
            }
        }
    }
    player_pose2d_t path;
    path.px = lr->getCoorValue(mainPath->x);
    path.py = lr->getCoorValue(mainPath->y);
    free(mainPath);
    cout << "Closest path is: (" << path.px << ", " << path.py << ")" << endl;
    return path;
}

/*
 * Overloading for the findPath method. Converts the coordinates to matrix values.
 */
bool Astar::findPath(double sx, double sy, double tx, double ty, vector<player_pose2d_t> *path) {
    return findPath(lr->getMatrixValue(sx), lr->getMatrixValue(sy), lr->getMatrixValue(tx), lr->getMatrixValue(ty), path);
}

/*
 * Main A* algorithm, takes a start coordinate (sx,sy) and a goal (tx,ty) and updates
 * the path vector to the best path it could find.
 * Returns false if no valid path was found.
 */
bool Astar::findPath(int sx, int sy, int tx, int ty, vector<player_pose2d_t> *path) {
    list<Node> closed;
    list<Node> open;
    int maxDepth = 0;
    open.clear();
    closed.clear();

    for (int x = 0; x < MAPSIZE_X; x++) {
        for (int y = 0; y < MAPSIZE_Y; y++) {
            nodes[x][y] = Node(x, y);
        }
    }

    for (int x = 0; x < MAPSIZE_X; x++) {
        for (int y = 0; y < MAPSIZE_Y; y++) {
            visited[x][y] = false;
        }
    }

    open.push_back(nodes[sx][sy]);
    nodes[sx][sy].inOpen = true;
    nodes[tx][ty].parent = NULL;

    cout << "Findpath while loop started" << endl;
    while ((maxDepth < MAX_DIST) && (open.size() != 0)) {
        open.sort();
        Node *current = &(open.front());
        if ((current->x == nodes[tx][ty].x) && (current->y == nodes[tx][ty].y)) {
            break;
        }
        open.remove(*current);
        closed.push_back(*current);
        current->inOpen = false;
        current->inClosed = true;

        if (visited[current->x][current->y]) continue;
        visited[current->x][current->y] = true;

        for (int x = -1; x < 2; x++) {
            for (int y = -1; y < 2; y++) {
                if ((x == 0) && (y == 0)) {
                    continue;
                }

                int xp = x + current->x;
                int yp = y + current->y;

                if (isValidLocation(sx, sy, xp, yp)) {
                    if (visited[xp][yp]) continue;
                    float nextStepCost = current->cost + getMovCost(current->x, current->y, xp, yp);
                    Node *neighbour = &nodes[xp][yp];

                    if (nextStepCost < neighbour->cost) {
                        if (inList(open, *neighbour)) {
                            open.remove(*neighbour);
                            neighbour->inOpen = false;
                        }
                        if (inList(closed, *neighbour)) {
                            closed.remove(*neighbour);
                            neighbour->inClosed = false;
                        }
                    }

                    if (!(neighbour-> inOpen) && !(neighbour->inClosed)) {
                        neighbour->cost = nextStepCost;
                        neighbour->heuristic = getHeuCost(xp, yp, tx, ty);
                        maxDepth = max(maxDepth, neighbour->setParent(current));
                        open.push_back(*neighbour);
                        neighbour->inOpen = true;
                    }
                }
            }
        }
    }
    cout << "Findpath while loop finished; depth: " << maxDepth << endl;
    if (nodes[tx][ty].parent == NULL) {
        return false;
    }

    Node *target = &nodes[tx][ty];
    cout << "Findpath adding path" << endl;
    path->clear();
    while ((target->x != nodes[sx][sy].x) || (target->y != nodes[sx][sy].y)) {

        path->push_back((player_pose2d_t) {
            lr->getCoorValue((double) target->x), lr->getCoorValue((double) target->y), 0.0
        });
        target = target->parent;
    }

    path->push_back((player_pose2d_t) {
        lr->getCoorValue((double) sx), lr->getCoorValue((double) sy), 0.0
    });
    return true;
}