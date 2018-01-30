#include <iostream>
#include <cmath>
#include <fstream>
#include <vector>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <ao/ao.h>
#include <mpg123.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define BITS 8

using namespace std;

struct VAO {
    GLuint VertexArrayID;
    GLuint VertexBuffer;
    GLuint ColorBuffer;

    GLenum PrimitiveMode;
    GLenum FillMode;
    int NumVertices;
};
typedef struct VAO VAO;

struct GLMatrices {
	glm::mat4 projection;
	glm::mat4 model;
	glm::mat4 view;
	GLuint MatrixID;
} Matrices;

GLuint programID;

/* Function to load Shaders - Use it as it is */
GLuint LoadShaders(const char * vertex_file_path,const char * fragment_file_path) {

	// Create the shaders
	GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
	GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

	// Read the Vertex Shader code from the file
	std::string VertexShaderCode;
	std::ifstream VertexShaderStream(vertex_file_path, std::ios::in);
	if(VertexShaderStream.is_open())
	{
		std::string Line = "";
		while(getline(VertexShaderStream, Line))
			VertexShaderCode += "\n" + Line;
		VertexShaderStream.close();
	}

	// Read the Fragment Shader code from the file
	std::string FragmentShaderCode;
	std::ifstream FragmentShaderStream(fragment_file_path, std::ios::in);
	if(FragmentShaderStream.is_open()){
		std::string Line = "";
		while(getline(FragmentShaderStream, Line))
			FragmentShaderCode += "\n" + Line;
		FragmentShaderStream.close();
	}

	GLint Result = GL_FALSE;
	int InfoLogLength;

	// Compile Vertex Shader
	printf("Compiling shader : %s\n", vertex_file_path);
	char const * VertexSourcePointer = VertexShaderCode.c_str();
	glShaderSource(VertexShaderID, 1, &VertexSourcePointer , NULL);
	glCompileShader(VertexShaderID);

	// Check Vertex Shader
	glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> VertexShaderErrorMessage(InfoLogLength);
	glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
	fprintf(stdout, "%s\n", &VertexShaderErrorMessage[0]);

	// Compile Fragment Shader
	printf("Compiling shader : %s\n", fragment_file_path);
	char const * FragmentSourcePointer = FragmentShaderCode.c_str();
	glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer , NULL);
	glCompileShader(FragmentShaderID);

	// Check Fragment Shader
	glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> FragmentShaderErrorMessage(InfoLogLength);
	glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
	fprintf(stdout, "%s\n", &FragmentShaderErrorMessage[0]);

	// Link the program
	fprintf(stdout, "Linking program\n");
	GLuint ProgramID = glCreateProgram();
	glAttachShader(ProgramID, VertexShaderID);
	glAttachShader(ProgramID, FragmentShaderID);
	glLinkProgram(ProgramID);

	// Check the program
	glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
	glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> ProgramErrorMessage( max(InfoLogLength, int(1)) );
	glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
	fprintf(stdout, "%s\n", &ProgramErrorMessage[0]);

	glDeleteShader(VertexShaderID);
	glDeleteShader(FragmentShaderID);

	return ProgramID;
}

static void error_callback(int error, const char* description)
{
    fprintf(stderr, "Error: %s\n", description);
}

void quit(GLFWwindow *window)
{
    glfwDestroyWindow(window);
    glfwTerminate();
//    exit(EXIT_SUCCESS);
}


/* Generate VAO, VBOs and return VAO handle */
struct VAO* create3DObject (GLenum primitive_mode, int numVertices, const GLfloat* vertex_buffer_data, const GLfloat* color_buffer_data, GLenum fill_mode=GL_FILL)
{
    struct VAO* vao = new struct VAO;
    vao->PrimitiveMode = primitive_mode;
    vao->NumVertices = numVertices;
    vao->FillMode = fill_mode;

    // Create Vertex Array Object
    // Should be done after CreateWindow and before any other GL calls
    glGenVertexArrays(1, &(vao->VertexArrayID)); // VAO
    glGenBuffers (1, &(vao->VertexBuffer)); // VBO - vertices
    glGenBuffers (1, &(vao->ColorBuffer));  // VBO - colors

    glBindVertexArray (vao->VertexArrayID); // Bind the VAO 
    glBindBuffer (GL_ARRAY_BUFFER, vao->VertexBuffer); // Bind the VBO vertices 
    glBufferData (GL_ARRAY_BUFFER, 3*numVertices*sizeof(GLfloat), vertex_buffer_data, GL_STATIC_DRAW); // Copy the vertices into VBO
    glVertexAttribPointer(
                          0,                  // attribute 0. Vertices
                          3,                  // size (x,y,z)
                          GL_FLOAT,           // type
                          GL_FALSE,           // normalized?
                          0,                  // stride
                          (void*)0            // array buffer offset
                          );

    glBindBuffer (GL_ARRAY_BUFFER, vao->ColorBuffer); // Bind the VBO colors 
    glBufferData (GL_ARRAY_BUFFER, 3*numVertices*sizeof(GLfloat), color_buffer_data, GL_STATIC_DRAW);  // Copy the vertex colors
    glVertexAttribPointer(
                          1,                  // attribute 1. Color
                          3,                  // size (r,g,b)
                          GL_FLOAT,           // type
                          GL_FALSE,           // normalized?
                          0,                  // stride
                          (void*)0            // array buffer offset
                          );

    return vao;
}

/* Generate VAO, VBOs and return VAO handle - Common Color for all vertices */
struct VAO* create3DObject (GLenum primitive_mode, int numVertices, const GLfloat* vertex_buffer_data, const GLfloat red, const GLfloat green, const GLfloat blue, GLenum fill_mode=GL_FILL)
{
    GLfloat* color_buffer_data = new GLfloat [3*numVertices];
    for (int i=0; i<numVertices; i++) 
    {
        color_buffer_data [3*i] = red;
        color_buffer_data [3*i + 1] = green;
        color_buffer_data [3*i + 2] = blue;
    }

    return create3DObject(primitive_mode, numVertices, vertex_buffer_data, color_buffer_data, fill_mode);
}

/* Render the VBOs handled by VAO */
void draw3DObject (struct VAO* vao)
{
    // Change the Fill Mode for this object
    glPolygonMode (GL_FRONT_AND_BACK, vao->FillMode);

    // Bind the VAO to use
    glBindVertexArray (vao->VertexArrayID);

    // Enable Vertex Attribute 0 - 3d Vertices
    glEnableVertexAttribArray(0);
    // Bind the VBO to use
    glBindBuffer(GL_ARRAY_BUFFER, vao->VertexBuffer);

    // Enable Vertex Attribute 1 - Color
    glEnableVertexAttribArray(1);
    // Bind the VBO to use
    glBindBuffer(GL_ARRAY_BUFFER, vao->ColorBuffer);

    // Draw the geometry !
    glDrawArrays(vao->PrimitiveMode, 0, vao->NumVertices); // Starting from vertex 0; 3 vertices total -> 1 triangle
}

/**************************
 * Customizable functions *
 **************************/

float triangle_rot_dir = 1;
float rectangle_rot_dir = 1;
bool triangle_rot_status = true;
bool rectangle_rot_status = true;

float q1=0, q2=0, q3=0, angle=0, zoom=0, pan=0;
int flag1=0, flag2=0, flag3;
/* Executed when a regular key is pressed/released/held-down */
/* Prefered for Keyboard events */
void keyboard (GLFWwindow* window, int key, int scancode, int action, int mods)
{
     // Function is called first on GLFW_PRESS.

  if(glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS && glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
  {
    q1=q1+0.2;
    flag3=1;
  }
  if(glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS && glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
  {
    q1=q1-0.2;
    flag3=1;
  }
  if(glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS && glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
  {
    q2=q2+0.2;
    flag3=1;
  }
  if(glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS && glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
  {
    q2=q2-0.2;
    flag3=1;
  }

  if(glfwGetKey(window, GLFW_KEY_S)==GLFW_PRESS)
  {
    q3=q3+0.2;
    flag3=0;
  }
  if(glfwGetKey(window, GLFW_KEY_F)==GLFW_PRESS)
  {
    q3=q3-0.2;
    flag3=0;
  }
  if(glfwGetKey(window, GLFW_KEY_A)==GLFW_PRESS)
  {
    angle=angle+5;
    flag3=0;
  }
  if(glfwGetKey(window, GLFW_KEY_D)==GLFW_PRESS)
  {
    angle=angle-5;
    flag3=0;
  }
  if(glfwGetKey(window, GLFW_KEY_UP)==GLFW_PRESS)
  {
    zoom=zoom-0.1;
    flag3=0;
  }
  if(glfwGetKey(window, GLFW_KEY_DOWN)==GLFW_PRESS)
  {
    zoom=zoom+0.1;
    flag3=0;
  }
  else if(glfwGetKey(window, GLFW_KEY_LEFT)==GLFW_PRESS && glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_RELEASE && glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_RELEASE)
  {
    pan=pan-0.1;
    flag3=0;
  }
  else if(glfwGetKey(window, GLFW_KEY_RIGHT)==GLFW_PRESS && glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_RELEASE && glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_RELEASE)
  {
    pan=pan+0.1;
    flag3=0;
  }

}

/* Executed for character input (like in text boxes) */
void keyboardChar (GLFWwindow* window, unsigned int key)
{
	switch (key) {
		case 'Q':
		case 'q':
            quit(window);
            break;
		default:
			break;
	}
}

/* Executed when a mouse button is pressed/released */
void mouseButton (GLFWwindow* window, int button, int action, int mods)
{
    switch (button) {
        case GLFW_MOUSE_BUTTON_LEFT:
            if (action == GLFW_RELEASE)
                triangle_rot_dir *= -1;
            break;
        case GLFW_MOUSE_BUTTON_RIGHT:
            if (action == GLFW_RELEASE) {
                rectangle_rot_dir *= -1;
            }
            break;
        default:
            break;
    }
}


/* Executed when window is resized to 'width' and 'height' */
/* Modify the bounds of the screen here in glm::ortho or Field of View in glm::Perspective */
void reshapeWindow (GLFWwindow* window, int width, int height)
{
    int fbwidth=width, fbheight=height;
    /* With Retina display on Mac OS X, GLFW's FramebufferSize
     is different from WindowSize */
    glfwGetFramebufferSize(window, &fbwidth, &fbheight);

	GLfloat fov = 90.0f;

	// sets the viewport of openGL renderer
	glViewport (0, 0, (GLsizei) fbwidth, (GLsizei) fbheight);

	// set the projection matrix as perspective
	/* glMatrixMode (GL_PROJECTION);
	   glLoadIdentity ();
	   gluPerspective (fov, (GLfloat) fbwidth / (GLfloat) fbheight, 0.1, 500.0); */
	// Store the projection matrix in a variable for future use
    // Perspective projection for 3D views
    // Matrices.projection = glm::perspective (fov, (GLfloat) fbwidth / (GLfloat) fbheight, 0.1f, 500.0f);

    // Ortho projection for 2D views
    Matrices.projection = glm::ortho(-4.0f-zoom+pan, 4.0f+zoom+pan, -4.0f-zoom, 4.0f+zoom, 0.1f, 500.0f);
}

VAO *triangle, *redbasket, *greenbasket, *shooter, *redbrick, *greenbrick, *blackbrick, *mirror1, *mirror2, *mirror3, *mirror4, *lazer, *stand;

// Creates the triangle object used in this sample code
void createShooter ()
{
  /* ONLY vertices between the bounds specified in glm::ortho will be visible on screen */

  /* Define vertex array as used in glBegin (GL_TRIANGLES) */
  static const GLfloat vertex_buffer_data [] = {
    0, 0.25,0, // vertex 0
    0,-0.25,0, // vertex 1
    0.5,0,0 // vertex 2
  };

  static const GLfloat color_buffer_data [] = {
    0.4,0,0.8, // color 0
    0.4,0,0.8,// color 1
    0.4,0,0.8 // color 2
  };

  // create3DObject creates and returns a handle to a VAO that can be used later
  shooter = create3DObject(GL_TRIANGLES, 3, vertex_buffer_data, color_buffer_data, GL_FILL);
}

// Creates the rectangle object used in this sample code
void createGreenBasket()
{
  // GL3 accepts only Triangles. Quads are not supported
  static const GLfloat vertex_buffer_data [] = {
    -0.5,-0.6,0, // vertex 1
    0.5,-0.6,0, // vertex 2
    0.5, 0.6,0, // vertex 3

    0.5, 0.6,0, // vertex 3
    -0.5, 0.6,0, // vertex 4
    -0.5,-0.6,0  // vertex 1
  };

  static const GLfloat color_buffer_data [] = {
    0,1,0, // color 1
    0,1,0, // color 2
    0,1,0, // color 3

    0,1,0, // color 3
    0,1,0, // color 4
    0,1,0  // color 1
  };

  // create3DObject creates and returns a handle to a VAO that can be used later
  greenbasket = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
}

void createGreenBrick()
{
  // GL3 accepts only Triangles. Quads are not supported
  static const GLfloat vertex_buffer_data [] = {
    -0.125,-0.15,0, // vertex 1
    0.125,-0.15,0, // vertex 2
    0.125, 0.15,0, // vertex 3

    0.125, 0.15,0, // vertex 3
    -0.125, 0.15,0, // vertex 4
    -0.125,-0.15,0  // vertex 1
  };

  static const GLfloat color_buffer_data [] = {
    0,1,0, // color 1
    0,1,0, // color 2
    0,1,0, // color 3

    0,1,0, // color 3
    0,1,0, // color 4
    0,1,0  // color 1
  };

  // create3DObject creates and returns a handle to a VAO that can be used later
  greenbrick = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
}

void createBlackBrick()
{
  // GL3 accepts only Triangles. Quads are not supported
  static const GLfloat vertex_buffer_data [] = {
    -0.125,-0.15,0, // vertex 1
    0.125,-0.15,0, // vertex 2
    0.125, 0.15,0, // vertex 3

    0.125, 0.15,0, // vertex 3
    -0.125, 0.15,0, // vertex 4
    -0.125,-0.15,0  // vertex 1
  };

  static const GLfloat color_buffer_data [] = {
    0,0,0, // color 1
    0,0,0, // color 2
    0,0,0, // color 3

    0,0,0, // color 3
    0,0,0, // color 4
    0,0,0  // color 1
  };

  // create3DObject creates and returns a handle to a VAO that can be used later
  blackbrick = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
}

void createRedBasket()
{
  // GL3 accepts only Triangles. Quads are not supported
  static const GLfloat vertex_buffer_data [] = {
    -0.5,-0.6,0, // vertex 1
    0.5,-0.6,0, // vertex 2
    0.5, 0.6,0, // vertex 3

    0.5, 0.6,0, // vertex 3
    -0.5, 0.6,0, // vertex 4
    -0.5,-0.6,0  // vertex 1
  };

  static const GLfloat color_buffer_data [] = {
    1,0,0, // color 1
    1,0,0, // color 2
    1,0,0, // color 3

    1,0,0, // color 3
    1,0,0, // color 4
    1,0,0  // color 1
  };

  // create3DObject creates and returns a handle to a VAO that can be used later
  redbasket = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
}

void createRedBrick()
{
  // GL3 accepts only Triangles. Quads are not supported
  static const GLfloat vertex_buffer_data [] = {
    -0.125,-0.15,0, // vertex 1
    0.125,-0.15,0, // vertex 2
    0.125, 0.15,0, // vertex 3

    0.125, 0.15,0, // vertex 3
    -0.125, 0.15,0, // vertex 4
    -0.125,-0.15,0  // vertex 1
  };

  static const GLfloat color_buffer_data [] = {
    1,0,0, // color 1
    1,0,0, // color 2
    1,0,0, // color 3

    1,0,0, // color 3
    1,0,0, // color 4
    1,0,0  // color 1
  };

  // create3DObject creates and returns a handle to a VAO that can be used later
  redbrick = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
}

void createMirror1()
{
  // GL3 accepts only Triangles. Quads are not supported
  static const GLfloat vertex_buffer_data [] = {
    -0.24,-0.3,0, // vertex 1
    0.3, 0.3,0, // vertex 2
    0.24, 0.3,0, // vertex 3

    0.24, 0.3,0, // vertex 3
    -0.3, -0.3,0, // vertex 4
    -0.24,-0.3,0  // vertex 1
  };

  static const GLfloat color_buffer_data [] = {
    0.6, 0.8, 1, // color 1
    0.6, 0.8, 1, // color 2
    0.6, 0.8, 1, // color 3

    0.6, 0.8, 1, // color 3
    0.6, 0.8, 1, // color 4
    0.6, 0.8, 1  // color 1
  };

  // create3DObject creates and returns a handle to a VAO that can be used later
  mirror1 = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
}

void createMirror2()
{
  // GL3 accepts only Triangles. Quads are not supported
  static const GLfloat vertex_buffer_data [] = {
    -0.24,-0.3,0, // vertex 1
    0.3, 0.3,0, // vertex 2
    0.24, 0.3,0, // vertex 3

    0.24, 0.3,0, // vertex 3
    -0.3, -0.3,0, // vertex 4
    -0.24,-0.3,0  // vertex 1
  };

  static const GLfloat color_buffer_data [] = {
    0.6, 0.8, 1, // color 1
    0.6, 0.8, 1, // color 2
    0.6, 0.8, 1, // color 3

    0.6, 0.8, 1, // color 3
    0.6, 0.8, 1, // color 4
    0.6, 0.8, 1  // color 1
  };

  // create3DObject creates and returns a handle to a VAO that can be used later
  mirror2 = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
}

void createMirror3()
{
  // GL3 accepts only Triangles. Quads are not supported
  static const GLfloat vertex_buffer_data [] = {
    0.3,-0.3,0, // vertex 1
    -0.24, 0.3,0, // vertex 2
    -0.3, 0.3,0, // vertex 3

    -0.3, 0.3,0, // vertex 3
    0.24, -0.3,0, // vertex 4
    0.3,-0.3,0  // vertex 1
  };

  static const GLfloat color_buffer_data [] = {
    0.6, 0.8, 1, // color 1
    0.6, 0.8, 1, // color 2
    0.6, 0.8, 1, // color 3

    0.6, 0.8, 1, // color 3
    0.6, 0.8, 1, // color 4
    0.6, 0.8, 1  // color 1
  };

  // create3DObject creates and returns a handle to a VAO that can be used later
  mirror3 = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
}

void createMirror4()
{
  // GL3 accepts only Triangles. Quads are not supported
  static const GLfloat vertex_buffer_data [] = {
    0.3,-0.3,0, // vertex 1
    -0.24, 0.3,0, // vertex 2
    -0.3, 0.3,0, // vertex 3

    -0.3, 0.3,0, // vertex 3
    0.24, -0.3,0, // vertex 4
    0.3,-0.3,0  // vertex 1
  };

  static const GLfloat color_buffer_data [] = {
    0.6, 0.8, 1, // color 1
    0.6, 0.8, 1, // color 2
    0.6, 0.8, 1, // color 3

    0.6, 0.8, 1, // color 3
    0.6, 0.8, 1, // color 4
    0.6, 0.8, 1  // color 1
  };

  // create3DObject creates and returns a handle to a VAO that can be used later
  mirror4 = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
}

void createLazer()
{
  // GL3 accepts only Triangles. Quads are not supported
  static const GLfloat vertex_buffer_data [] = {
    -0.15,0.03,0, // vertex 1
    -0.15, -0.03,0, // vertex 2
    0.15, -0.03,0, // vertex 3

    0.15, -0.03,0, // vertex 3
    0.15, 0.03,0, // vertex 4
    -0.15,0.03,0  // vertex 1
  };

  static const GLfloat color_buffer_data [] = {
    0.8, 0.4, 1, // color 1
    0.8, 0.4, 1, // color 2
    0.8, 0.4, 1, // color 3

    0.8, 0.4, 1, // color 3
    0.8, 0.4, 1, // color 4
    0.8, 0.4, 1  // color 1
  };

  // create3DObject creates and returns a handle to a VAO that can be used later
  lazer = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
}

void createStand()
{
  // GL3 accepts only Triangles. Quads are not supported
  static const GLfloat vertex_buffer_data [] = {
    -0.125,0.125,0, // vertex 1
    -0.125, -0.125,0, // vertex 2
    0.125, -0.125,0, // vertex 3

    0.125, -0.125,0, // vertex 3
    0.125, 0.125,0, // vertex 4
    -0.125,0.125,0  // vertex 1
  };

  static const GLfloat color_buffer_data [] = {
    0.8, 0.4, 1, // color 1
    0.8, 0.4, 1, // color 2
    0.8, 0.4, 1, // color 3

    0.8, 0.4, 1, // color 3
    0.8, 0.4, 1, // color 4
    0.8, 0.4, 1  // color 1
  };

  // create3DObject creates and returns a handle to a VAO that can be used later
  stand = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
}

float camera_rotation_angle = 90;
float red_rotation = 0;
float redbri_rotation = 0;
float green_rotation = 0;
float greenbri_rotation = 0;
float blackbri_rotation = 0;
float bri_rotation = 0;
float shooter_rotation = 0;
float mirror1_rotation = 0;
float mirror2_rotation = 0;
float mirror3_rotation = 0;
float mirror4_rotation = 0;
float lazer_rotation=0;
float stand_rotation=0;

/* Render the scene with openGL */
/* Edit this function according to your assignment */
int n=0, ct=0, n1=0, ct1=0;
vector<float> x,y,z, r, s, t, u, e, p, f, h;
float a,b,c, a1, b1, c1, d1, s_x, s_y, t_r, m1, m2,m3,m4,m5,m6,m7,m8;
int delay = 0, life=5, score=0, penalty=0;

void draw (GLFWwindow* window)
{
  //flag3=0;
  // clear the color and depth in the frame buffer
  glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // use the loaded shader program
  // Don't change unless you know what you are doing
  glUseProgram (programID);

  // Eye - Location of camera. Don't change unless you are sure!!
  glm::vec3 eye ( 5*cos(camera_rotation_angle*M_PI/180.0f), 0, 5*sin(camera_rotation_angle*M_PI/180.0f) );
  // Target - Where is the camera looking at.  Don't change unless you are sure!!
  glm::vec3 target (0, 0, 0);
  // Up - Up vector defines tilt of camera.  Don't change unless you are sure!!
  glm::vec3 up (0, 1, 0);

  // Compute Camera matrix (view)
  // Matrices.view = glm::lookAt( eye, target, up ); // Rotating Camera for 3D
  //  Don't change unless you are sure!!
  Matrices.view = glm::lookAt(glm::vec3(0,0,3), glm::vec3(0,0,0), glm::vec3(0,1,0)); // Fixed camera for 2D (ortho) in XY plane

  // Compute ViewProject matrix as view/camera might not be changed for this frame (basic scenario)
  //  Don't change unless you are sure!!
  glm::mat4 VP = Matrices.projection * Matrices.view;

  // Send our transformation to the currently bound shader, in the "MVP" uniform
  // For each model you render, since the MVP will be different (at least the M part)
  //  Don't change unless you are sure!!
  glm::mat4 MVP;	// MVP = Projection * View * Model

  ct++;
  if(ct==90)
  {
    a=rand()%7 +0.85;
    a=a-3;
    b=4.2;
    c=rand()%3;
    x.push_back(a);
    y.push_back(b);
    z.push_back(c);
    e.push_back(1);
    ct=0;
    n++;
  }
  delay++;

  if(glfwGetKey(window, GLFW_KEY_SPACE)==GLFW_PRESS && delay >= 60)
  {
    delay = 0;
    a1=0;
    b1=q3;
    c1=angle;
    d1=sin(angle * M_PI/180.0f) * 0.45;
    r.push_back(a1);
    s.push_back(b1);
    t.push_back(c1);
    u.push_back(d1);
    p.push_back(1);
    f.push_back(0);
    h.push_back(0);
    n1++;
  }

  if(-2+q1>=3.5)
  {
    q1=q1-0.2;
  }
  if(-2+q1<= -2)
  {
    q1=q1+0.2;
  }
  if(2+q2>=3.5)
  {
    q2=q2-0.2;
  }
  if(2+q2<= -2)
  {
    q2=q2+0.2;
  }
  if(q3>=3.75)
  {
    q3=q3-0.2;
  }
  if(q3<= -3.75)
  {
    q3=q3+0.2;
  }
  if(angle>=80)
  {
    angle=angle-5;
  }
  if(angle<= -80)
  {
    angle=angle+5;
  }

  // Load identity to model matrix
  Matrices.model = glm::mat4(1.0f);

  /* Render your scene */
  shooter_rotation=angle;

  glm::mat4 translateTriangle = glm::translate (glm::vec3(-3.75f, q3, 0.0f)); // glTranslatef
  glm::mat4 rotateTriangle = glm::rotate((float)(shooter_rotation*M_PI/180.0f), glm::vec3(0,0,1));  // rotate about vector (1,0,0)
  glm::mat4 triangleTransform = translateTriangle * rotateTriangle;
  Matrices.model *= triangleTransform; 
  MVP = VP * Matrices.model; // MVP = p * V * M

  //  Don't change unless you are sure!!
  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

  // draw3DObject draws the VAO given to it using current MVP matrix
  draw3DObject(shooter);

  for(int i=0; i<n1; i++)
  {
    if(p[i] == 1)
    {
      s_x= -3.45 + f[i] + r[i] * cos((lazer_rotation*M_PI/180.0f)); //+ 0.15*cos((lazer_rotation*M_PI/180.0f));
      s_y= s[i] + u[i] + h[i] + r[i] * sin((lazer_rotation*M_PI/180.0f)); //+ 0.15*sin((lazer_rotation*M_PI/180.0f)) ;
      for(int w=0; w<n;w++)
      {
        if(e[w] == 1)
        {
          if((s_x >= x[w]-0.125 && s_x <= x[w]+0.125)&&(s_y >= y[w]-0.15 && s_y <= y[w]+0.15))
          {
            e[w]=0;
            p[i]=0;
            if(z[w]==0)
            {
              life=life-1;
            }
            else if(z[w]==1)
            {
              life=life-1;
            }
            else if(z[w]==2)
            {
              score=score+1;
            }
            if(life==0)
            {
              cout<<"\nLives Over!! GAME OVER!!!\n";
              exit(0);
            }
            cout<<"\nScore : "<<score<<"\nLife : "<<life<<endl;
          }
        }
      }
    }
  }
  for(int j=0;j<n1;j++)
  {  
    if(p[j]==1)
    {
      lazer_rotation=t[j];
      Matrices.model = glm::mat4(1.0f);
      glm::mat4 translateRectangle11 = glm::translate (glm::vec3(-3.45f+f[j], s[j] + u[j] + h[j], 0));        // glTranslatef
      glm::mat4 rotateRectangle11 = glm::rotate((float)(lazer_rotation*M_PI/180.0f), glm::vec3(0,0,1)); // rotate about vector (-1,1,1)
      glm::mat4 translateRectangle12 = glm::translate (glm::vec3(r[j], 0, 0));        // glTranslatef
      Matrices.model *= (translateRectangle11 * rotateRectangle11 * translateRectangle12);
      MVP = VP * Matrices.model;
      glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
      // draw3DObject draws the VAO given to it using current MVP matrix
      draw3DObject(lazer);
      r[j]=r[j]+0.1;
    }
  }
  for(int i=0; i<n1; i++)
  {
    if(p[i]==1)
    {
      s_x= -3.45 + f[i] + r[i] * cos((lazer_rotation*M_PI/180.0f)); //+ 0.15*cos((lazer_rotation*M_PI/180.0f));
      s_y= s[i] + u[i] + h[i] + r[i] * sin((lazer_rotation*M_PI/180.0f)); //+ 0.15*sin((lazer_rotation*M_PI/180.0f)) ;
      m1=1.11;
      m2=(s_y+1.7)/(s_x-3.24);
      m3=1.11;
      m4=(s_y-0.3)/(s_x-0.24);
      m5=-1.11;
      m6=(s_y-3.3)/(s_x-2.7);
      m7=-1.11;
      m8=(s_y-3.3)/(s_x+0.3);
      if((m2<=m1+0.5 && m2>=m1-0.5) && (s_x<=3.24 && s_x>=2.7) && (s_y >= -2.3 && s_y<= -1.7))
      {
        f[i]=s_x+3.45;
        h[i]=s_y-(s[i]+u[i]);
        r[i]=0;
        t[i]=t[i]+96;
      }
      else if((m4<=m3+0.5 && m4>=m3-0.5) && (s_x<=0.24 && s_x>=-0.3) && (s_y >= -0.3 && s_y<= 0.3))
      {
        f[i]=s_x+3.45;
        h[i]=s_y-(s[i]+u[i]);
        r[i]=0;
        t[i]=t[i]+96;
      }
      else if((m6<=m5+0.5 && m6>=m5-0.5) && (s_x<=3.24 && s_x>=2.7) && (s_y >= 2.7 && s_y<= 3.3))
      {
        f[i]=s_x+3.45;
        h[i]=s_y-(s[i]+u[i]);
        r[i]=0;
        t[i]=t[i]+144;
      }
      else if((m8<=m7+0.5 && m8>=m7-0.5) && (s_x<=0.24 && s_x>=-0.3) && (s_y >= 2.7 && s_y<= 3.3))
      {
        f[i]=s_x+3.45;
        h[i]=s_y-(s[i]+u[i]);
        r[i]=0;
        t[i]=t[i]+144;
      }
    }

    /*for(int w=0; w<n;w++)
    {
      if((s_x >= x[w]-0.125 && s_x <= x[w]+0.125)&&(s_y >= y[w]-0.15 && s_y <= y[w]+0.15))
      {
        e[w]=0;
        p[w]=0;
      }
    }*/

  }

  Matrices.model = glm::mat4(1.0f);

  glm::mat4 translateRectangle14 = glm::translate (glm::vec3(-3.875f, q3, 0.0f));        // glTranslatef
  glm::mat4 rotateRectangle14 = glm::rotate((float)(green_rotation*M_PI/180.0f), glm::vec3(0,0,1)); // rotate about vector (-1,1,1)
  Matrices.model *= (translateRectangle14 * rotateRectangle14);
  MVP = VP * Matrices.model;
  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

  // draw3DObject draws the VAO given to it using current MVP matrix
  draw3DObject(stand);

  // Pop matrix to undo transformations till last push matrix instead of recomputing model matrix
  // glPopMatrix ();
  Matrices.model = glm::mat4(1.0f);

  glm::mat4 translateRectangle1 = glm::translate (glm::vec3(2+q2, -3.4, 0));        // glTranslatef
  glm::mat4 rotateRectangle1 = glm::rotate((float)(green_rotation*M_PI/180.0f), glm::vec3(0,0,1)); // rotate about vector (-1,1,1)
  Matrices.model *= (translateRectangle1 * rotateRectangle1);
  MVP = VP * Matrices.model;
  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

  // draw3DObject draws the VAO given to it using current MVP matrix
  draw3DObject(greenbasket);

  for(int k=0; k<n; k++)
  {
    if(e[k] == 1)
    {
      if((x[k] >= -2.5+q1 && x[k] <= -1.5+q1 && y[k] <= -2.82 && y[k] >= -2.85))
      {
        e[k]=0;
        if(z[k]==0)
        {
          score=score+1;
        }
        else if(z[k]==1)
        {
          score=score-1;
        }
        else if(z[k]==2)
        {
          cout<<"\nBlack Brick in the Hole!! GAME OVER!!!\n";
          exit(0);
        }
        cout<<"\nScore : "<<score<<"\nLife : "<<life<<endl;
      }
      if((x[k] >= 1.5+q2 && x[k] <= 2.5+q2 && y[k] <= -2.82 && y[k] >= -2.85))
      {
        e[k]=0;
        if(z[k]==0)
        {
          score=score-1;
        }
        else if(z[k]==1)
        {
          score=score+1;
        }
        else if(z[k]==2)
        {
          cout<<"\nBlack Brick in the Hole!! GAME OVER!!!\n";
          exit(0);
        }
        cout<<"\nScore : "<<score<<"\nLife : "<<life<<endl;
      }
    }
  }
  for(int i=0;i<n;i++)
  {  
    
    Matrices.model = glm::mat4(1.0f);

    glm::mat4 translateRectangle3 = glm::translate (glm::vec3(x[i], y[i], 0));        // glTranslatef
    glm::mat4 rotateRectangle3 = glm::rotate((float)(bri_rotation*M_PI/180.0f), glm::vec3(0,0,1)); // rotate about vector (-1,1,1)
    Matrices.model *= (translateRectangle3 * rotateRectangle3);
    MVP = VP * Matrices.model;
    glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
    if(z[i]==0)
    {
      if(e[i]==1)
      {
        // draw3DObject draws the VAO given to it using current MVP matrix
        draw3DObject(redbrick);
      }
    }
    else if(z[i]==1)
    {
      if(e[i]==1)
      {
        // draw3DObject draws the VAO given to it using current MVP matrix
        draw3DObject(greenbrick);
      }
    }
    else if(z[i]==2)
    {
      if(e[i]==1)
      {
        // draw3DObject draws the VAO given to it using current MVP matrix
        draw3DObject(blackbrick);
      }
    }
    if(glfwGetKey(window, GLFW_KEY_N)==GLFW_PRESS)
    {
      y[i]=y[i]-0.1;
    }
    else if(glfwGetKey(window, GLFW_KEY_M)==GLFW_PRESS)
    {
      y[i]=y[i]-0.002;
    }
    else
    {
      y[i]=y[i]-0.007;
    }
  }

  Matrices.model = glm::mat4(1.0f);

  glm::mat4 translateRectangle2 = glm::translate (glm::vec3(-2+q1, -3.4, 0));        // glTranslatef
  glm::mat4 rotateRectangle2 = glm::rotate((float)(red_rotation*M_PI/180.0f), glm::vec3(0,0,1)); // rotate about vector (-1,1,1)
  Matrices.model *= (translateRectangle2 * rotateRectangle2);
  MVP = VP * Matrices.model;
  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

  // draw3DObject draws the VAO given to it using current MVP matrix
  draw3DObject(redbasket);

  Matrices.model = glm::mat4(1.0f);

  glm::mat4 translateRectangle4 = glm::translate (glm::vec3(-2, 3, 0));        // glTranslatef
  glm::mat4 rotateRectangle4 = glm::rotate((float)(redbri_rotation*M_PI/180.0f), glm::vec3(0,0,1)); // rotate about vector (-1,1,1)
  Matrices.model *= (translateRectangle4 * rotateRectangle4);
  MVP = VP * Matrices.model;
  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

  // draw3DObject draws the VAO given to it using current MVP matrix
  // draw3DObject(redbrick);

  Matrices.model = glm::mat4(1.0f);

  glm::mat4 translateRectangle5 = glm::translate (glm::vec3(2, 3, 0));        // glTranslatef
  glm::mat4 rotateRectangle5 = glm::rotate((float)(blackbri_rotation*M_PI/180.0f), glm::vec3(0,0,1)); // rotate about vector (-1,1,1)
  Matrices.model *= (translateRectangle5 * rotateRectangle5);
  MVP = VP * Matrices.model;
  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

  // draw3DObject draws the VAO given to it using current MVP matrix
  // draw3DObject(blackbrick);

  Matrices.model = glm::mat4(1.0f);

  glm::mat4 translateRectangle6 = glm::translate (glm::vec3(3, -2, 0));        // glTranslatef
  glm::mat4 rotateRectangle6 = glm::rotate((float)(mirror1_rotation*M_PI/180.0f), glm::vec3(0,0,1)); // rotate about vector (-1,1,1)
  Matrices.model *= (translateRectangle6 * rotateRectangle6);
  MVP = VP * Matrices.model;
  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

  // draw3DObject draws the VAO given to it using current MVP matrix
  draw3DObject(mirror1);

  Matrices.model = glm::mat4(1.0f);

  glm::mat4 translateRectangle7 = glm::translate (glm::vec3(0, 0, 0));        // glTranslatef
  glm::mat4 rotateRectangle7 = glm::rotate((float)(mirror2_rotation*M_PI/180.0f), glm::vec3(0,0,1)); // rotate about vector (-1,1,1)
  Matrices.model *= (translateRectangle7 * rotateRectangle7);
  MVP = VP * Matrices.model;
  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

  // draw3DObject draws the VAO given to it using current MVP matrix
  draw3DObject(mirror2);

  Matrices.model = glm::mat4(1.0f);

  glm::mat4 translateRectangle8 = glm::translate (glm::vec3(3, 3, 0));        // glTranslatef
  glm::mat4 rotateRectangle8 = glm::rotate((float)(mirror3_rotation*M_PI/180.0f), glm::vec3(0,0,1)); // rotate about vector (-1,1,1)
  Matrices.model *= (translateRectangle8 * rotateRectangle8);
  MVP = VP * Matrices.model;
  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

  // draw3DObject draws the VAO given to it using current MVP matrix
  draw3DObject(mirror3);

  Matrices.model = glm::mat4(1.0f);

  glm::mat4 translateRectangle9 = glm::translate (glm::vec3(0, 3, 0));        // glTranslatef
  glm::mat4 rotateRectangle9 = glm::rotate((float)(mirror4_rotation*M_PI/180.0f), glm::vec3(0,0,1)); // rotate about vector (-1,1,1)
  Matrices.model *= (translateRectangle9 * rotateRectangle9);
  MVP = VP * Matrices.model;
  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

  // draw3DObject draws the VAO given to it using current MVP matrix
  draw3DObject(mirror4);


  // Increment angles
  float increments = 0;

  //camera_rotation_angle++; // Simulating camera rotation
  shooter_rotation = shooter_rotation + increments*triangle_rot_dir*triangle_rot_status;
  red_rotation = red_rotation + increments*rectangle_rot_dir*rectangle_rot_status;
  redbri_rotation = redbri_rotation + increments*rectangle_rot_dir*rectangle_rot_status;
  green_rotation = green_rotation + increments*rectangle_rot_dir*rectangle_rot_status;
  greenbri_rotation = greenbri_rotation + increments*rectangle_rot_dir*rectangle_rot_status;
  blackbri_rotation = blackbri_rotation + increments*rectangle_rot_dir*rectangle_rot_status;
  bri_rotation = bri_rotation + increments*rectangle_rot_dir*rectangle_rot_status;
  mirror1_rotation = mirror1_rotation + increments*rectangle_rot_dir*rectangle_rot_status;
  mirror2_rotation = mirror2_rotation + increments*rectangle_rot_dir*rectangle_rot_status;
  mirror3_rotation = mirror3_rotation + increments*rectangle_rot_dir*rectangle_rot_status;
  mirror4_rotation = mirror4_rotation + increments*rectangle_rot_dir*rectangle_rot_status;
  lazer_rotation = lazer_rotation + increments*rectangle_rot_dir*rectangle_rot_status;
  stand_rotation = stand_rotation + increments*rectangle_rot_dir*rectangle_rot_status;
}

/* Initialise glfw window, I/O callbacks and the renderer to use */
/* Nothing to Edit here */
GLFWwindow* initGLFW (int width, int height)
{
    GLFWwindow* window; // window desciptor/handle

    glfwSetErrorCallback(error_callback);
    if (!glfwInit()) {
//        exit(EXIT_FAILURE);
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(width, height, "Brick Breaker", NULL, NULL);

    if (!window) {
        glfwTerminate();
//        exit(EXIT_FAILURE);
    }

    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
    glfwSwapInterval( 1 );

    /* --- register callbacks with GLFW --- */

    /* Register function to handle window resizes */
    /* With Retina display on Mac OS X GLFW's FramebufferSize
     is different from WindowSize */
    glfwSetFramebufferSizeCallback(window, reshapeWindow);
    glfwSetWindowSizeCallback(window, reshapeWindow);

    /* Register function to handle window close */
    glfwSetWindowCloseCallback(window, quit);

    /* Register function to handle keyboard input */
    glfwSetKeyCallback(window, keyboard);      // general keyboard input
    glfwSetCharCallback(window, keyboardChar);  // simpler specific character handling

    /* Register function to handle mouse click */
    glfwSetMouseButtonCallback(window, mouseButton);  // mouse button clicks

    return window;
}

/* Initialize the OpenGL rendering properties */
/* Add all the models to be created here */
void initGL (GLFWwindow* window, int width, int height)
{
    /* Objects should be created before any other gl function and shaders */
	// Create the models
	createShooter (); // Generate the VAO, VBOs, vertices data & copy into the array buffer
	createRedBasket ();
  createRedBrick ();
  createGreenBasket ();
  createGreenBrick ();
  createBlackBrick ();
  createMirror1 ();
  createMirror2 ();
  createMirror3 ();
  createMirror4 ();
  createLazer();
  createStand();
	
	// Create and compile our GLSL program from the shaders
	programID = LoadShaders( "Sample_GL.vert", "Sample_GL.frag" );
	// Get a handle for our "MVP" uniform
	Matrices.MatrixID = glGetUniformLocation(programID, "MVP");

	
	reshapeWindow (window, width, height);

    // Background color of the scene
	glClearColor (1.0f, 1.0f, 0.8f, 1.0f); // R, G, B, A
	glClearDepth (1.0f);

	glEnable (GL_DEPTH_TEST);
	glDepthFunc (GL_LEQUAL);

    cout << "VENDOR: " << glGetString(GL_VENDOR) << endl;
    cout << "RENDERER: " << glGetString(GL_RENDERER) << endl;
    cout << "VERSION: " << glGetString(GL_VERSION) << endl;
    cout << "GLSL: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;
}

int main (int argc, char** argv)
{
	int width = 600;
	int height = 600;

    GLFWwindow* window = initGLFW(width, height);

	initGL (window, width, height);

    double last_update_time = glfwGetTime(), current_time;
    cout<<"\nScore : "<<score<<"\nLife : "<<life<<endl;

    mpg123_handle *mh;
    unsigned char *buffer;
    size_t buffer_size;
    size_t done;
    int err;

    int driver;
    ao_device *dev;

    ao_sample_format format;
    int channels, encoding;
    long rate;

    //if(argc < 2)
    //    exit(0);

    /* initializations */
    ao_initialize();
    driver = ao_default_driver_id();
    mpg123_init();
    mh = mpg123_new(NULL, &err);
    buffer_size = 3000;
    buffer = (unsigned char*) malloc(buffer_size * sizeof(unsigned char));

    /* open the file and get the decoding format */
    mpg123_open(mh, "spooky1.mp3");
    mpg123_getformat(mh, &rate, &channels, &encoding);

    /* set the output format and open the output device */
    format.bits = mpg123_encsize(encoding) * BITS;
    format.rate = rate;
    format.channels = channels;
    format.byte_format = AO_FMT_NATIVE;
    format.matrix = 0;
    dev = ao_open_live(driver, &format, NULL);

    /* Draw in loop */
    while (!glfwWindowShouldClose(window)) {
        /* decode and play */
          if (mpg123_read(mh, buffer, buffer_size, &done) == MPG123_OK)
              ao_play(dev, (char *)buffer, done);
          else mpg123_seek(mh, 0, SEEK_SET); // loop audio from start again if ended
        // OpenGL Draw commands
        draw(window);

        reshapeWindow (window, width, height);

        // Swap Frame Buffer in double buffering
        glfwSwapBuffers(window);

        // Poll for Keyboard and mouse events
        glfwPollEvents();

        // Control based on time (Time based transformation like 5 degrees rotation every 0.5s)
        current_time = glfwGetTime(); // Time in seconds
        if ((current_time - last_update_time) >= 0.5) { // atleast 0.5s elapsed since last frame
            // do something every 0.5 seconds ..
            last_update_time = current_time;
        }
    }
    /* clean up */
    free(buffer);
    ao_close(dev);
    mpg123_close(mh);
    mpg123_delete(mh);
    mpg123_exit();
    ao_shutdown();


    glfwTerminate();
    return 0; 
//    exit(EXIT_SUCCESS);
}
