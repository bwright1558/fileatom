/* fileatom 3D filesystem visualizer
 * Copyright (C) 2013 Brian Wright
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see [http://www.gnu.org/licenses/].
 */

#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glu.h>
#include <GL/glut.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <unistd.h>
#include <dirent.h>
#include <string>
using namespace std;

#define PI 3.14159265f
#define VIEW_ALL 0
#define VIEW_FILES 1
#define VIEW_DIRS 2
typedef float vec3[3];
typedef float vec4[4];
typedef float vec16[16];

static int winid;
static float transx = 0.0f, transy = 0.0f, transz = -4.0f;
static float radius = 1.0f, fileradius = 0.1f, speed = 0.002f;
static bool orbit = false, randaxes = false, text = false, firstframe = true;
static bool sphere = false, cube = false;
static int view = VIEW_ALL;
static vec4 qclick = {1.0f, 0.0f, 0.0f, 0.0f};
static vec4 qdrag = {1.0f, 0.0f, 0.0f, 0.0f};
static vec4 qlast = {1.0f, 0.0f, 0.0f, 0.0f};
static vec4 rlast = {1.0f, 0.0f, 0.0f, 0.0f};

static int nfiles = 0;
static int selfile = 0;
static bool *isdir = NULL, *posangle = NULL;
static string *files = NULL;
static float *angles = NULL;
static vec3 *filepts = NULL, *axes = NULL;
static vec4 *qclicks = NULL, *qdrags = NULL, *qlasts = NULL, *rlasts = NULL;

float randf(float a, float b);
void updatefiles();
void updateaxes();
void updatefilepts();
void arcball(int x, int h, vec4 q);
void zero(vec4 q);
void inverse(vec4 q, vec4 r);
void copy(vec4 q, vec4 r);
void multiply(vec4 q1, vec4 q2, vec4 r);
void rotationmatrix(vec4 q, vec16 r);
void neworientation(vec4 qi, vec4 qf, vec4 old, vec4 r);
void drawtext(int n, float x, float y, float z, const char *text);
void initglut(int argc, char **argv);
void initgl();
void reshape(int w, int h);
void mouse(int button, int state, int x, int y);
void motion(int x, int y);
void passivemotion(int x, int y);
void idle();
void keyboard(unsigned char key, int x, int y);
void special(int key, int x, int y);
void display();

int main(int argc, char **argv) {
    int opt;
    srand(time(NULL));
    while ((opt = getopt(argc, argv, "otr:s:p:x:y:z:h")) != -1) {
        switch (opt) {
            case 'o': orbit = true; break;
            case 't': text = true; break;
            case 'r': radius = atof(optarg); break;
            case 's': speed = atof(optarg); break;
            case 'p': chdir(optarg); break;
            case 'x': transx = -atof(optarg); break;
            case 'y': transy = -atof(optarg); break;
            case 'z': transz = -atof(optarg); break;
            case 'h': printf("usage: %s [-o] [-t] [-r RADIUS] [-s SPEED] [-x TRANSX] [-y TRANSY] [-z TRANSZ] [-p PATH] [-h]\n", argv[0]); exit(0); break;
            default:  printf("usage: %s [-o] [-t] [-r RADIUS] [-s SPEED] [-x TRANSX] [-y TRANSY] [-z TRANSZ] [-p PATH] [-h]\n", argv[0]); exit(1); break;
        }
    }
    updatefiles();
    initglut(argc, argv);
    initgl();
    glutMainLoop();
    return 0;
}

float randf(float a, float b) {
    return (((float)rand())/(float)RAND_MAX)*(b - a) + a;
}

void updatefiles() {
    int i;
    struct dirent *dp;
    char *path = get_current_dir_name();
    DIR *dir = opendir(path);
    DIR *testdir = NULL;
    i = nfiles = selfile = 0;
    delete[] isdir; isdir = NULL;
    delete[] posangle; posangle = NULL;
    delete[] files; files = NULL;
    delete[] angles; angles = NULL;
    delete[] filepts; filepts = NULL;
    delete[] axes; axes = NULL;
    delete[] qclicks; qclicks = NULL;
    delete[] qdrags; qdrags = NULL;
    delete[] qlasts; qlasts = NULL;
    delete[] rlasts; rlasts = NULL;
    while ((dp = readdir(dir)) != NULL) {
        if (view == VIEW_ALL) nfiles += 1;
        else {
            if ((testdir = opendir(dp->d_name)) != NULL) {
                closedir(testdir); testdir = NULL;
                if (view == VIEW_DIRS) nfiles += 1;
            } else if (view == VIEW_FILES) nfiles += 1;
        }
    }
    rewinddir(dir);
    isdir = new bool[nfiles];
    posangle = new bool[nfiles];
    files = new string[nfiles];
    angles = new float[nfiles];
    filepts = new vec3[nfiles];
    axes = new vec3[nfiles];
    qclicks = new vec4[nfiles];
    qdrags = new vec4[nfiles];
    qlasts = new vec4[nfiles];
    rlasts = new vec4[nfiles];
    while ((dp = readdir(dir)) != NULL) {
        if ((testdir = opendir(dp->d_name)) != NULL) {
            if (view == VIEW_ALL || view == VIEW_DIRS) {
                isdir[i] = true;
                files[i] = string(dp->d_name);
                closedir(testdir); testdir = NULL;
                i++;
            }
        } else if (view == VIEW_ALL || view == VIEW_FILES) {
            isdir[i] = false;
            files[i] = string(dp->d_name);
            i++;
        }
    }
    closedir(dir); dir = NULL;
    free(path); path = NULL;
    updatefilepts();
}

void updateaxes() {
    int i;
    float mag;
    vec3 v;
    firstframe = true;
    for (i = 0; i < nfiles; i++) {
        if (randaxes) {
            axes[i][0] = randf(-radius, radius);
            axes[i][1] = randf(-radius, radius);
            axes[i][2] = randf(-radius, radius);
        } else {
            v[0] = randf(-radius, radius);
            v[1] = randf(-radius, radius);
            v[2] = randf(-radius, radius);
            axes[i][0] = filepts[i][1]*v[2] - filepts[i][2]*v[1];
            axes[i][1] = filepts[i][2]*v[0] - filepts[i][0]*v[2];
            axes[i][2] = filepts[i][0]*v[1] - filepts[i][1]*v[0];
        }
        mag = sqrt(axes[i][0]*axes[i][0] + axes[i][1]*axes[i][1]
                                         + axes[i][2]*axes[i][2]);
        axes[i][0] /= mag;
        axes[i][1] /= mag;
        axes[i][2] /= mag;
        angles[i] = 0.0f;
        zero(qclicks[i]);
        zero(qdrags[i]);
        zero(qlasts[i]);
        zero(rlasts[i]);
        if (randf(0.0f, 1.0f) <= 0.5f) posangle[i] = true;
        else                           posangle[i] = false;
    }
}

void updatefilepts() {
    int i;
    float r, phi;
    float goldenangle = PI*(3.0f - sqrt(5.0f));
    float off = 2.0f/(float)nfiles;
    for (i = 0; i < nfiles; i++) {
        filepts[i][1] = (float)i*off - 1.0f + (off/2.0f);
        r = sqrt(1.0f - filepts[i][1]*filepts[i][1]);
        phi = (float)i*goldenangle;
        filepts[i][0] = r*cos(phi);
        filepts[i][2] = r*sin(phi);
    }
    updateaxes();
}

void arcball(int x, int y, vec4 q) {
    float mag, mag2;
    int w, h;
    int viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    w = viewport[2];
    h = viewport[3];
    q[0] = 0.0f;
    q[1] = 2.0f*(float)x/(float)w - 1.0f;
    q[2] = 2.0f*(float)(h - y)/(float)h - 1.0f;
    q[3] = 0.0f;
    mag2 = q[0]*q[0] + q[1]*q[1] + q[2]*q[2] + q[3]*q[3];
    mag = sqrt(mag2);
    if (mag2 > 1.0f) {
        q[1] /= mag;
        q[2] /= mag;
    } else q[3] = sqrt(1.0f - mag2);
}

void zero(vec4 q) {
    q[0] = 1.0f;
    q[1] = q[2] = q[3] = 0.0f;
}

void inverse(vec4 q, vec4 r) {
    r[0] = q[0];
    r[1] = -q[1];
    r[2] = -q[2];
    r[3] = -q[3];
}

void copy(vec4 q, vec4 r) {
    r[0] = q[0];
    r[1] = q[1];
    r[2] = q[2];
    r[3] = q[3];
}

void multiply(vec4 q1, vec4 q2, vec4 r) {
    r[0] = q1[0]*q2[0] - q1[1]*q2[1] - q1[2]*q2[2] - q1[3]*q2[3];
    r[1] = q1[0]*q2[1] + q1[1]*q2[0] + q1[2]*q2[3] - q1[3]*q2[2];
    r[2] = q1[0]*q2[2] + q1[2]*q2[0] + q1[3]*q2[1] - q1[1]*q2[3];
    r[3] = q1[0]*q2[3] + q1[3]*q2[0] + q1[1]*q2[2] - q1[2]*q2[1];
}

void rotationmatrix(vec4 q, vec16 r) {
    r[0]  = 1.0f - 2.0f*q[2]*q[2] - 2.0f*q[3]*q[3];
    r[1]  = 2.0f*q[1]*q[2] + 2.0f*q[0]*q[3];
    r[2]  = 2.0f*q[1]*q[3] - 2.0f*q[0]*q[2];
    r[3]  = 0.0f;
    r[4]  = 2.0f*q[1]*q[2] - 2.0f*q[0]*q[3];
    r[5]  = 1.0f - 2.0f*q[1]*q[1] - 2.0f*q[3]*q[3];
    r[6]  = 2.0f*q[2]*q[3] + 2.0f*q[0]*q[1];
    r[7]  = 0.0f;
    r[8]  = 2.0f*q[1]*q[3] + 2.0f*q[0]*q[2];
    r[9]  = 2.0f*q[2]*q[3] - 2.0f*q[0]*q[1];
    r[10] = 1.0f - 2.0f*q[1]*q[1] - 2.0f*q[2]*q[2];
    r[11] = 0.0f;
    r[12] = 0.0f;
    r[13] = 0.0f;
    r[14] = 0.0f;
    r[15] = 1.0f;
}

void neworientation(vec4 qi, vec4 qf, vec4 old, vec4 r) {
    vec4 qiinv, qfqiinv, qfqiinvold;
    inverse(qi, qiinv);
    multiply(qf, qiinv, qfqiinv);
    multiply(qfqiinv, old, qfqiinvold);
    copy(qfqiinvold, r);
}

void drawtext(int n, float x, float y, float z, const char *text) {
    int i = 0;
    glRasterPos3f(x, y, z);
    for (i = 0; i < n; i++)
        glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, text[i]);
}

void initglut(int argc, char **argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA | GLUT_ALPHA | GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowSize(500, 500);
    glutInitWindowPosition(0, 0);
    winid = glutCreateWindow(argv[0]);
    glutReshapeFunc(reshape);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    glutPassiveMotionFunc(passivemotion);
    glutIdleFunc(idle);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(special);
    glutDisplayFunc(display);
}

void initgl() {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_POINT_SMOOTH);
    glEnable(GL_LIGHT0);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_LIGHTING);
    glDisable(GL_COLOR_MATERIAL);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glLineWidth(0.7f);
    glPointSize(3.0f);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
}

void reshape(int w, int h) {
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60, (float)w/(float)h, 0.1f, 2000);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void mouse(int button, int state, int x, int y) {
    switch (state) {
        case GLUT_DOWN:
            arcball(x, y, qlast);
            copy(qlast, qdrag);
            copy(qlast, qclick);
            break;
        case GLUT_UP:
            neworientation(qclick, qdrag, rlast, rlast);
            zero(qdrag);
            zero(qclick);
            break;
        default: break;
    }
    glutPostRedisplay();
}

void motion(int x, int y) {
    copy(qdrag, qlast);
    arcball(x, y, qdrag);
    glutPostRedisplay();
}

void passivemotion(int x, int y) {
}

void idle() {
    int i;
    float angleover2, sinangle;
    if (!orbit) return;
    for (i = 0; i < nfiles; i++) {
        if (posangle[i]) {
            angles[i] += 2.0f*PI*speed;
            if (angles[i] >= 2.0f*PI) angles[i] = speed;
        } else {
            angles[i] -= 2.0f*PI*speed;
            if (angles[i] <= 0.0f) angles[i] = 2.0f*PI;
        }
        angleover2 = angles[i]/2.0f;
        sinangle = sin(angleover2);
        if (firstframe) {
            firstframe = false;
            qlasts[i][0] = cos(angleover2);
            qlasts[i][1] = sinangle*axes[i][0];
            qlasts[i][2] = sinangle*axes[i][1];
            qlasts[i][3] = sinangle*axes[i][2];
            copy(qlasts[i], qdrags[i]);
            copy(qlasts[i], qclicks[i]);
        } else {
            copy(qdrags[i], qlasts[i]);
            qdrags[i][0] = cos(angleover2);
            qdrags[i][1] = sinangle*axes[i][0];
            qdrags[i][2] = sinangle*axes[i][1];
            qdrags[i][3] = sinangle*axes[i][2];
        }
    }
    glutPostRedisplay();
}

void keyboard(unsigned char key, int x, int y) {
    switch (key) {
        case 'q':
            glutDestroyWindow(winid);
            delete[] isdir; isdir = NULL;
            delete[] posangle; posangle = NULL;
            delete[] files; files = NULL;
            delete[] angles; angles = NULL;
            delete[] filepts; filepts = NULL;
            delete[] axes; axes = NULL;
            delete[] qclicks; qclicks = NULL;
            delete[] qdrags; qdrags = NULL;
            delete[] qlasts; qlasts = NULL;
            delete[] rlasts; rlasts = NULL;
            exit(0);
            break;
        case 'o': orbit = !orbit; break;
        case '<': if (radius > 0.0f) radius -= 0.1f; break;
        case '>': radius += 0.1f; break;
        case '[': if (speed > 0.0f) speed -= 0.001f; break;
        case ']': speed += 0.001f; break;
        case 'n': updateaxes(); break;
        case 'r': randaxes = !randaxes; updateaxes(); break;
        case 'a': view = VIEW_ALL; updatefiles(); break;
        case 'f': view = VIEW_FILES; updatefiles(); break;
        case 'd': view = VIEW_DIRS; updatefiles(); break;
        case 'g': if (isdir[selfile]) {
                      chdir(files[selfile].c_str());
                      updatefiles();
                  }
                  break;
        case 't': text = !text; break;
        case 'j': if (selfile == nfiles - 1) selfile = 0; else selfile++;
                  break;
        case 'k': if (selfile == 0) selfile = nfiles - 1; else selfile--;
                  break;
        case 's': sphere = !sphere; break;
        case 'c': cube = !cube; break;
        case '+': fileradius += 0.01f; break;
        case '-': if (fileradius > 0.0f) fileradius -= 0.01f; break;
        default: break;
    }
    glutPostRedisplay();
}

void special(int key, int x, int y) {
    switch (key) {
        case GLUT_KEY_UP: transz += 0.1f; break;
        case GLUT_KEY_DOWN: transz -= 0.1f; break;
        case GLUT_KEY_LEFT:  transx += 0.1f; break;
        case GLUT_KEY_RIGHT: transx -= 0.1f; break;
        case GLUT_KEY_PAGE_UP: transy -= 0.1f; break;
        case GLUT_KEY_PAGE_DOWN: transy += 0.1f; break;
        default: break;
    }
    glutPostRedisplay();
}

void display() {
    int i;
    vec4 q;
    vec16 r;
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glPushMatrix();
    neworientation(qclick, qdrag, rlast, q);
    rotationmatrix(q, r);
    glTranslatef(transx, transy, transz);
    glMultMatrixf(r);
    if (cube) {
        glEnable(GL_LIGHTING);
        glutSolidCube(0.20f);
        glDisable(GL_LIGHTING);
    }
    if (sphere) glutWireSphere(radius, 15, 15);
    for (i = 0; i < nfiles; i++) {
        glPushMatrix();
        neworientation(qclicks[i], qdrags[i], rlasts[i], q);
        rotationmatrix(q, r);
        glMultMatrixf(r);
        if (text) drawtext(files[i].length(),
                           1.2f*radius*filepts[i][0],
                           1.2f*radius*filepts[i][1],
                           1.2f*radius*filepts[i][2],
                           files[i].c_str());
        glTranslatef(radius*filepts[i][0],
                     radius*filepts[i][1],
                     radius*filepts[i][2]);
        if (i == selfile) {
            glDisable(GL_DEPTH_TEST);
            glColor4f(1.0f, 0.0f, 1.0f, 0.3f);
            glutSolidSphere(fileradius/3.0f + fileradius, 25, 25);
            glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
            glEnable(GL_DEPTH_TEST);
        }
        glEnable(GL_LIGHTING);
        if (isdir[i]) glutSolidSphere(fileradius, 25, 25);
        else glutSolidCube(fileradius);
        glDisable(GL_LIGHTING);
        glPopMatrix();
    }
    glPopMatrix();
    glutSwapBuffers();
}

