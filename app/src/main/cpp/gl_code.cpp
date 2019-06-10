/*
 * Copyright (C) 2009 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// OpenGL ES 2.0 code

#include <jni.h>
#include <android/log.h>

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define  LOG_TAG    "libgl2jni"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

static void printGLString(const char *name, GLenum s) {
    const char *v = (const char *) glGetString(s);
    LOGI("GL %s = %s\n", name, v);
}

static void checkGlError(const char* op) {
    for (GLint error = glGetError(); error; error
            = glGetError()) {
        LOGI("after %s() glError (0x%x)\n", op, error);
    }
}

auto gVertexShader =
    "attribute vec4 vPosition;\n"
    "uniform mat4 mvp;\n"
    "void main() {\n"
    "  gl_Position = mvp*vPosition;\n"
    "}\n";

auto gFragmentShader =
    "precision mediump float;\n"
    "void main() {\n"
    "  gl_FragColor = vec4(0.0, 1.0, 0.0, 1.0);\n"
    "}\n";

GLuint loadShader(GLenum shaderType, const char* pSource) {
    GLuint shader = glCreateShader(shaderType);
    if (shader) {
        glShaderSource(shader, 1, &pSource, NULL);
        glCompileShader(shader);
        GLint compiled = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
        if (!compiled) {
            GLint infoLen = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
            if (infoLen) {
                char* buf = (char*) malloc(infoLen);
                if (buf) {
                    glGetShaderInfoLog(shader, infoLen, NULL, buf);
                    LOGE("Could not compile shader %d:\n%s\n",
                            shaderType, buf);
                    free(buf);
                }
                glDeleteShader(shader);
                shader = 0;
            }
        }
    }
    return shader;
}

GLuint createProgram(const char* pVertexSource, const char* pFragmentSource) {
    GLuint vertexShader = loadShader(GL_VERTEX_SHADER, pVertexSource);
    if (!vertexShader) {
        return 0;
    }

    GLuint pixelShader = loadShader(GL_FRAGMENT_SHADER, pFragmentSource);
    if (!pixelShader) {
        return 0;
    }

    GLuint program = glCreateProgram();
    if (program) {
        glAttachShader(program, vertexShader);
        checkGlError("glAttachShader");
        glAttachShader(program, pixelShader);
        checkGlError("glAttachShader");
        glLinkProgram(program);
        GLint linkStatus = GL_FALSE;
        glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
        if (linkStatus != GL_TRUE) {
            GLint bufLength = 0;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &bufLength);
            if (bufLength) {
                char* buf = (char*) malloc(bufLength);
                if (buf) {
                    glGetProgramInfoLog(program, bufLength, NULL, buf);
                    LOGE("Could not link program:\n%s\n", buf);
                    free(buf);
                }
            }
            glDeleteProgram(program);
            program = 0;
        }
    }
    return program;
}

GLuint gProgram;
GLuint gvPositionHandle;
GLuint gmvpHandle;

bool setupGraphics(int w, int h) {
    printGLString("Version", GL_VERSION);
    printGLString("Vendor", GL_VENDOR);
    printGLString("Renderer", GL_RENDERER);
    printGLString("Extensions", GL_EXTENSIONS);

    LOGI("setupGraphics(%d, %d)", w, h);
    gProgram = createProgram(gVertexShader, gFragmentShader);
    if (!gProgram) {
        LOGE("Could not create program.");
        return false;
    }
    gvPositionHandle = glGetAttribLocation(gProgram, "vPosition");
    checkGlError("glGetAttribLocation");
    LOGI("glGetAttribLocation(\"vPosition\") = %d\n",
            gvPositionHandle);
    gmvpHandle = glGetUniformLocation(gProgram, "mvp");
    checkGlError("glGetUniformLocation");

    glViewport(0, 0, w, h);
    checkGlError("glViewport");
    return true;
}

const GLfloat gTriangleVertices[] = {
        0.5f, 0.5f, -0.5f,  0.5f, -0.5f, -0.5f, -0.5, 0.5f, -0.5f, -0.5f, -0.5f, -0.5f, //First side
        -0.5f, 0.5f, 0.5f, -0.5f, -0.5f, 0.5f, //Second side
        0.5f, 0.5f, 0.5f, 0.5f, -0.5f, 0.5f, //Third side
        0.5f, 0.5f, -0.5f, 0.5f, -0.5f, -0.5f, //Fourth side
        0.5f, 0.5f, 0.5f,  0.5f, 0.5f, -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f, -0.5f, //Top
        0.5f, -0.5f, 0.5f,  0.5f, -0.5f, -0.5f, -0.5f, -0.5f, 0.5f, -0.5f, -0.5f, -0.5f, //Bottom
        };
GLfloat mvp[] = {1.f, 0, 0 ,0,
                     0, 1.f, 0, 0,
                     0, 0, 1.f, 0,
                     0, 0, 0, 1.f};

void setEulerAngles(GLfloat *m, float omega, float phi, float kappa) {
    m[0] = cos(phi);            m[1] = -cos(kappa)*sin(phi);                                 m[2] = sin(phi)*sin(kappa);                                   m[3] = 0;
    m[4] = cos(omega)*sin(phi); m[5] = cos(omega)*cos(phi)*cos(kappa)-sin(omega)*sin(kappa); m[6] = -cos(kappa)*sin(omega)-cos(omega)*cos(phi)*sin(kappa); m[7] = 0;
    m[8] = sin(omega)*sin(phi); m[9] = cos(omega)*sin(kappa)+cos(phi)*cos(kappa)*sin(omega); m[10] = cos(omega)*cos(kappa)-cos(phi)*sin(omega)*sin(kappa); m[11] = 0;
    m[12] = 0;                  m[13] = 0;                                                   m[14] = 0;                                                    m[15] = 1;
}

void setProj(GLfloat *m) {
    m[0] = 1.f;     m[1] = 0;       m[2] = 0;       m[3] = 0;
    m[4] = 0;       m[5] = 1.f;     m[6] = 0;       m[7] = 0;
    m[8] = 0;       m[9] = 0;       m[10] = -1010.f/990;    m[11] = -20000/990.f;
    m[12] = 0;      m[13] = 0;      m[14] = -1.f;      m[15] = 0;
}

void setShift(GLfloat *m, GLfloat x, GLfloat y, GLfloat z) {
    m[0] = 1.f;     m[1] = 0;       m[2] = 0;       m[3] = x;
    m[4] = 0;       m[5] = 1.f;     m[6] = 0;       m[7] = y;
    m[8] = 0;       m[9] = 0;       m[10] = 1.f;    m[11] = z;
    m[12] = 0;      m[13] = 0;      m[14] = 0;      m[15] = 1.f;
}


void multMat(GLfloat *z, const GLfloat *x, const GLfloat *y) {
    for(int i=0; i<4; i++)
        for(int j=0; j<4; j++) {
            z[i * 4 + j] = 0;
            for (int k = 0; k < 4; k++)
                z[i * 4 + j] += x[i * 4 + k] * y[k * 4 + j];
        }
}

void renderFrame() {
    static float grey;
    static float omega=0.0, phi=0.0, kappa=0.0;
    GLfloat cameraMat[16], modelMat[16], projMat[16], modelCam[16];
    grey += 0.01f;
    if (grey > 1.0f) {
        grey = 0.0f;
    }
    omega += 0.02; phi += 0.03; kappa += 0.025;
    if(omega >= M_PI)
        omega = -M_PI;
    if(phi >= M_PI)
        phi = -M_PI;
    if(kappa >= M_PI)
        kappa = -M_PI;
    setEulerAngles(modelMat, omega, phi, kappa);
    setProj(projMat);
    setShift(cameraMat, 0, 0, -15.5f);
    multMat(modelCam, cameraMat, modelMat);
    multMat(mvp, projMat, modelCam);
    glClearColor(grey, grey, grey, 1.0f);
    checkGlError("glClearColor");
    glClear( GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    checkGlError("glClear");

    glUseProgram(gProgram);
    checkGlError("glUseProgram");

    glVertexAttribPointer(gvPositionHandle, 3, GL_FLOAT, GL_FALSE, 0, gTriangleVertices);
    checkGlError("glVertexAttribPointer");
    glEnableVertexAttribArray(gvPositionHandle);
    checkGlError("glEnableVertexAttribArray");
    glUniformMatrix4fv(gmvpHandle, 1, GL_FALSE, mvp);
    checkGlError("glUniformMatrix4fv");
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 10); //Sides
    glDrawArrays(GL_TRIANGLE_STRIP, 10, 4); //Top
    glDrawArrays(GL_TRIANGLE_STRIP, 14, 4); //Bottom
    checkGlError("glDrawArrays");
}

extern "C" {
    JNIEXPORT void JNICALL Java_com_android_gl2jni_GL2JNILib_init(JNIEnv * env, jobject obj,  jint width, jint height);
    JNIEXPORT void JNICALL Java_com_android_gl2jni_GL2JNILib_step(JNIEnv * env, jobject obj);
};

JNIEXPORT void JNICALL Java_com_android_gl2jni_GL2JNILib_init(JNIEnv * env, jobject obj,  jint width, jint height)
{
    setupGraphics(width, height);
}

JNIEXPORT void JNICALL Java_com_android_gl2jni_GL2JNILib_step(JNIEnv * env, jobject obj)
{
    renderFrame();
}
