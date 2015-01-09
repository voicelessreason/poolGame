/******************************************************************************
 * Arlen Strausman
 * 
 * This is a Pool simulation game written for my computer graphics course.
 * It is built upon a collision detection and animation program 
 * designed by Thomas Kelliher. 
 * 
 * Functions written: createAimer, rackBoard, shoot, rackCue, aim, raisePower,
 * lowerPower, elasiticityUp, elasticityDown, ballSizeUp, ballSizeDown, moveCueUp
 * moveCueDown, moveCueLeft, moveCueRight,
 * 
 * Functions modified: readFile, createCircle, createBoard, display, init, 
 * keyboard, mouse, idle, main
 * 
 * ****************************************************************************
 *  Tom Kelliher, Goucher College
 *  Feb. 15, 2013
 *  collision.cpp
 *
 *  This is a simple double buffered program that demonstrates double
 *  buffering and animation.  More importantly, it demonstrates collision
 *  detection and response for spheres (here, 2-D balls).  This works fine
 *  assuming we don't have "too many" collisions at one time.
 ******************************************************************************/

#ifdef WIN32
#include <Windows.h>
#include <Winbase.h>
#else
#include <sys/time.h>
int GetTickCount()
{
struct timeval tv;
gettimeofday(&tv, NULL);
return 1000 * tv.tv_sec + tv.tv_usec / 1000;
}
#endif

#define DOUBLE_BUFFER

/* Some basic constants.  MAX_BALLS is rather meaningless at this point.
 * ESC is the ASCII value of the Esc key.  ELASTICITY is used to define
 * the elasticity of collisions.  It may range between 1.0 (completely
 * elastic) to 0.0 (completely inelastic).  VELOCITY_SCALE is used to scale
 * velocity to a reasonable value on fast machines.  SLICES is the number
 * of vertices to generate for rendering a ball, which is rendered as a
 * circle.
 */

const int MAX_BALLS = 100;
const int ESC = 0x1b;
const float VELOCITY_SCALE = 0.01;
const int SLICES = 72;
int count = 0;

#include <time.h>
#include "Angel.h"
#include <fstream>


/* Identifiers for the shader programs and the uniform projection and model
 * view matrices in the vertex shader (see vshader41.glsl).
 */

GLuint program;
GLuint projection;
GLuint model_view;


/* Basic data structures for the simulation. */

typedef vec3 Color;


/* Most of these are self-explanatory.  vao is the indentifier for the vertex
 * array object holding the vertex attributes for this ball.  numVertices
 * is the number of vertices represented within the VAO.  geometry is the
 * geometry (GL_LINES, GL_TRIANGLES, etc.) to use when drawing the VAO.
 */

typedef struct Ball
{
   vec2 position;
   vec2 oPosition;
   vec2 velocity;
   GLdouble radius;
   GLdouble mass;
   Color color;
   GLuint vao;
   GLuint numVertices;
   GLuint geometry;
   int isPocket;
   int isIgnored;
   int hasBeenShot;
} Ball;

double fringeWidth;
double friction;
double ELASTICITY;
int displayThreshold;
int numBalls;
int numPockets;
int currentTick = -1;
GLuint boardVAO;
GLuint boardBuffer;
vec2 points[4];
vec2 aimValue;
int powerValue;
Color colors[4];
vec2 ll, ur;
Color boardColor, fringeColor;
Ball aimBall, aimCircle;

/* Initial window width and height */

GLuint windowWidth = 900;
GLuint windowHeight = 450;

/* We use a 1:1 aspect ratio.  For convenience in setting the projection
 * matrix, WORLD_HALF is half the clipping region's width/height.  Refer
 * to display().
 */ 

const GLfloat WORLD_HALF = 50.0;
GLfloat worldRight = WORLD_HALF;
GLfloat worldTop = WORLD_HALF;


/***********************************************************************
 * Prototypes for basic vector operations not already defined in vec.h.
 ***********************************************************************/

GLfloat distanceSquared(vec2 v);


/***********************************************************************
 * Prototypes for collision detection and response, and for setting
 * attributes of the simulation objects.
 ***********************************************************************/

int collision(Ball ball1, Ball ball2);
void collisionResponse(Ball& ball1, Ball& ball2);
void placeBalls(void);
void shoot(void);
void initBalls(void);
void rackBoard(void);
void rackCue(void);
void elasticityDown(void);
void elasticityUp(void);
void raisePower(void);
void lowerPower(void);
void moveCueDown(void);
void moveCueUp(void);
void moveCueForward(void);
void moveCueBack(void);
void ballSizeUp(void);
void ballSizeDown(void);
vec2 aim(void);
GLuint createCircle(Ball ball);


/***********************************************************************
 * Prototypes for the basic OpenGL functions.
 ***********************************************************************/

void display(void);
void init(void);
void reshape(int w, int h);
void idle(void);
void keyboard(unsigned char key, int x, int y);


/* Data structure for holding the simulation objects. */

Ball balls[MAX_BALLS];


/***********************************************************************
 * Definitions for basic vector operations.
 ***********************************************************************/


/***********************************************************************
 * We use distanceSquared() wherever we can to avoid computing a square
 * root (expensive).
 ***********************************************************************/

GLfloat distanceSquared(vec2 v)
{
   return dot(v, v);
}


/***********************************************************************
 * Collision detection and response functions.
 ***********************************************************************/

	int collision(Ball ball1, Ball ball2)
	{
	   double radiusSum = ball1.radius + ball2.radius;

	   /* Vector from center of ball2 to center of ball1.  This vector is
		* normal to the collision plane.
		*/

	   vec2 collisionNormal = ball1.position - ball2.position;

	   /* Note that we're comparing square of distance, to avoid computing
		* square roots.  We've had a collision if the distance between
		* the centers of the balls is <= to the sum of their radii.
		*/
	   if(ball1.isPocket == 1)
	   {
		   return (distanceSquared(collisionNormal) <= ball1.radius * ball1.radius)
				  ? 1 : 0;
	   }else if (ball2.isPocket == 1)
		{
		   return (distanceSquared(collisionNormal) <= ball2.radius * ball2.radius)
				 ? 1 : 0;
		}else{
		   return (distanceSquared(collisionNormal) <= radiusSum * radiusSum)
				 ? 1 : 0;
		}
	}

/***********************************************************************
 * File Reading
 ***********************************************************************/
void readFile()
{
	std::ifstream data;
	data.open("poolData.txt");
	if(!data.is_open())
	{
		std::cout << "Could not open dat file, dawg" << std::endl;
		exit(1);
	}
	data >> displayThreshold;
	data >> ll.x;
	data >> ll.y;
	data >> ur.x;
	data >> ur.y;
	data >> boardColor;
	data >> fringeWidth;
	data >> fringeColor;
	data >> ELASTICITY;
	data >> friction;
	data >> powerValue;
	data >> numBalls;

	double zC[2*numBalls];
	int z = 0;
	for(int i = 0; i < numBalls; i++)
	{
		balls[i].isPocket = 0;
		balls[i].isIgnored = 0;
		data >> balls[i].mass;
		data >> balls[i].radius;
		data >> balls[i].color;
		data >> balls[i].position;
		balls[i].oPosition = balls[i].position;
		data >> zC[z]; z++;
		data >> balls[i].velocity;
		data >> zC[z]; z++;
	}
	data >> numPockets;

	double yC[2*numPockets];
	int y = 0;
	for(int i = numBalls; i < (numPockets + numBalls); i++)
	{
		balls[i].isPocket = 1;
		balls[i].isIgnored = 0;
		data >> balls[i].mass;
		data >> balls[i].radius;
		data >> balls[i].color;
		data >> balls[i].position;
		balls[i].oPosition = balls[i].position;
		data >> yC[y]; y++;
		data >> balls[i].velocity;
		data >> yC[y]; y++;
	}
	data.close();
}


/***********************************************************************
 * We may have to make modifications to ball1 and ball2, so we need to
 * pass in references to them.  This function will determine the
 * response to the collision and modify each ball's position and
 * velocity vector to account for the collision response.
 ***********************************************************************/

void collisionResponse(Ball& ball1, Ball& ball2)
{
	if(ball1.isPocket == 1)
	{
		ball2.velocity = 0.0;
		ball2.position = -ball1.oPosition * 2;
	}
	else if(ball2.isPocket == 1)
	{
		ball1.velocity = 0.0;
		ball1.position = -ball1.oPosition * 2;
	}else if(ball1.isPocket == 0 || ball2.isPocket == 0){
   double radiusSum = ball1.radius + ball2.radius;

   /* Vector from center of ball2 to center of ball1.  This vector is
    * normal to the collision plane.
    */

   vec2 collisionNormal = ball1.position - ball2.position;

   /* Penetration distance is sum of radii less distance between centers
    * of the two balls.
    */

   double distance = length(collisionNormal);
   double penetration = radiusSum - distance;

   vec2 relativeVelocity = ball2.velocity - ball1.velocity;

   /* Dot product of relative velocity and collision normal.  If this
    * is negative, the balls are already moving apart, and we need not
    * compute a collision response.
    */

   double vDOTn;  

   /* The following are used to compute the collision impulse.  This is
    * energy added to each ball to draw them apart following the collision.
    * The total energy in the system remains the same, or is less than
    * before the collision if the collision is inelastic.
    */

   double numerator;
   double denominator;
   double impulse;

   collisionNormal = normalize(collisionNormal);

   /* Readjust ball position by translating each ball by 1/2 the
    * penetration distance along the collision normal.
    */

   ball1.position = ball1.position + 0.5 * penetration * collisionNormal;

   ball2.position = ball2.position - 0.5 * penetration * collisionNormal;

   vDOTn = dot(relativeVelocity, collisionNormal);

   if (vDOTn < 0.0)
      return;

   /* Compute impulse energy. */

   numerator = -(1.0 + ELASTICITY) * vDOTn;
   denominator = (1.0 / ball2.mass + 1.0 / ball1.mass);
   impulse = numerator / denominator;

   /* Apply the impulse to each ball. */

   ball2.velocity = ball2.velocity + impulse / ball2.mass * collisionNormal;

   ball1.velocity = ball1.velocity - impulse / ball1.mass * collisionNormal;
	}
}

/***********************************************************************
 * Assign initial attributes to the two balls.  This data should really
 * be read from a file.
 ***********************************************************************/

  void initBalls(void)
{
   //Create geometry information and vaos for each ball.

   for (int i = 0; i < MAX_BALLS; ++i)
   {
	  if(balls[i].isIgnored == 0)
	   {
		  balls[i].vao = createCircle(balls[i]);
		  balls[i].geometry = GL_TRIANGLE_FAN;
		  balls[i].numVertices = SLICES;
	   }
   }
}


/***********************************************************************
 * Create and setup a complete vao, buffer, and set of shader programs
 * to render the given ball as a circle.
 ***********************************************************************/

GLuint createCircle(Ball ball)
{
   GLuint vao, buffer;

   vec2 points[SLICES];
   vec3 colors[SLICES];
   GLfloat angle = 0.0;
   GLfloat sliceAngle = 2.0 * M_PI / (GLfloat) SLICES;

   for (int i = 0; i < SLICES; i++)
   {
      points[i] = ball.radius * vec2(cos(angle), sin(angle));
      colors[i] = ball.color;
      angle += sliceAngle;
   }

   glGenVertexArrays(1, &vao);
   glBindVertexArray(vao);

   glGenBuffers(1, &buffer);
   glBindBuffer(GL_ARRAY_BUFFER, buffer);

   glBufferData(GL_ARRAY_BUFFER, sizeof(points) + sizeof(colors),
                NULL, GL_STATIC_DRAW);

   glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(points), points);
   glBufferSubData(GL_ARRAY_BUFFER, sizeof(points), sizeof(colors), colors);

   glUseProgram(program);

   GLuint vPosition = glGetAttribLocation(program, "vPosition");
   glEnableVertexAttribArray(vPosition);
   glVertexAttribPointer(vPosition, 2, GL_FLOAT, GL_FALSE, 0,
                         BUFFER_OFFSET(0));

   GLuint vColor = glGetAttribLocation(program, "vColor");
   glEnableVertexAttribArray(vColor);
   glVertexAttribPointer(vColor, 3, GL_FLOAT, GL_FALSE, 0,
                         BUFFER_OFFSET(sizeof(points)));
   return vao;
}
/***********************************************************************
* Create and setup a complete vao, buffer, and set of shader programs
 * to render the board
 ***********************************************************************/
GLuint createBoard()
{
	int boardIndex = 0;

	points[boardIndex] = vec2( ll.x, ll.y ); colors[boardIndex] = boardColor; boardIndex++;
	points[boardIndex] = vec2( ur.x, ll.y ); colors[boardIndex] = boardColor; boardIndex++;
	points[boardIndex] = vec2( ur.x, ur.y ); colors[boardIndex] = boardColor; boardIndex++;
	points[boardIndex] = vec2( ll.x, ur.y  ); colors[boardIndex] = boardColor; boardIndex++;


		   // Create a vertex array object
		   glGenVertexArrays( 1, &boardVAO );
		   glBindVertexArray( boardVAO );

		   // Create and initialize a buffer object
		   glGenBuffers( 1, &boardBuffer );
		   glBindBuffer( GL_ARRAY_BUFFER, boardBuffer );

		   glBufferData( GL_ARRAY_BUFFER, sizeof(points) + sizeof(colors),
						 NULL, GL_STATIC_DRAW );

		   glBufferSubData( GL_ARRAY_BUFFER, 0, sizeof(points), points );
		   glBufferSubData( GL_ARRAY_BUFFER, sizeof(points), sizeof(colors), colors );

		   GLuint vPosition = glGetAttribLocation(program, "vPosition");
		   glEnableVertexAttribArray(vPosition);
		   glVertexAttribPointer(vPosition, 2, GL_FLOAT, GL_FALSE, 0,
		                            BUFFER_OFFSET(0));

		   GLuint vColor = glGetAttribLocation(program, "vColor");
		   glEnableVertexAttribArray(vColor);
		   glVertexAttribPointer(vColor, 3, GL_FLOAT, GL_FALSE, 0,
		                            BUFFER_OFFSET(sizeof(points)));

		   return boardVAO;
}
/***********************************************************************
 * Create and set up aiming circle
 *
 ***********************************************************************/
void createAimer()
{
	balls[2].oPosition.x = -100.0; balls[2].oPosition.y = -100.0;
	balls[0].geometry = GL_TRIANGLE_FAN;
	balls[1].geometry = GL_TRIANGLE_FAN;
	balls[2].geometry = GL_LINE_LOOP;
	balls[3].geometry = GL_TRIANGLE_FAN;
	balls[4].hasBeenShot = 0;

	for(int i = 0; i < 4; i++)
	{
		balls[i].isIgnored = 1;
		balls[i].numVertices = SLICES;
		balls[i].vao = createCircle(balls[i]);
	}
}

/***********************************************************************
 * OpenGL functions.
 ***********************************************************************/

/***********************************************************************
 * Recall, this will do our rendering for us.  It is called following
 * each simulation step in order to update the window.
 ***********************************************************************/

void display(void)
{
   mat4 mv;   /* Model view matrix */
   mat4 p;    /* Projection matrix */

   glClear(GL_COLOR_BUFFER_BIT);

   /* Define the projection matrix and make it available to the vertex
    * shader.
    */

   p = Ortho(ll.x-fringeWidth, ur.x+fringeWidth, ll.y-fringeWidth, ur.y+fringeWidth, -1.0, 1.0);
   glUniformMatrix4fv(projection, 1, GL_TRUE, p);

   // Render Board //

   glBindVertexArray(boardVAO);

   glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    // Swap buffers, for smooth animation.  This will also flush the
   // * pipeline.


   /* Render balls. */


   for (int i = 0; i < MAX_BALLS; i++)
   {
      glBindVertexArray(balls[i].vao);

      /* Define the object-appropriate model view matrix and make it
       *  available to the vertex shader.
       */

      mv = Translate(balls[i].position.x, balls[i].position.y, 0.0);
      glUniformMatrix4fv(model_view, 1, GL_TRUE, mv);

      glDrawArrays(balls[i].geometry, 0, balls[i].numVertices);
   }

   glutSwapBuffers();
}


void init(void) 
{
   readFile();
   glClearColor (fringeColor.x, fringeColor.y, fringeColor.z, 0.0);
   glShadeModel (GL_FLAT);   /* Probably unnecessary. */

   /* Load shaders and use the resulting shader program. */

   program = InitShader("vshader41.glsl", "fshader41.glsl");

   /* Get the locations of the uniform matrices in the vertex shader. */

   model_view = glGetUniformLocation( program, "model_view" );
   projection = glGetUniformLocation( program, "projection" );

   createAimer();
   initBalls();
   createBoard();

}


/***********************************************************************
 * Hitting the Esc key will exit the program.
 ***********************************************************************/

void keyboard(unsigned char key, int x, int y)
{
   switch (key)
   {
      case ESC:
          exit(0);
          break;
      case ' ':
    	  shoot();
    	  break;
      case 'r':
    	  rackBoard();
    	  break;
      case 'c':
    	  rackCue();
    	  break;
      case 'E':
    	  elasticityUp();
    	  break;
      case 'e':
    	  elasticityDown();
    	  break;
      case 'w':
    	  moveCueUp();
    	  break;
      case 'b':
    	  ballSizeDown();
    	  break;
      case 'B':
    	  ballSizeUp();
    	  break;
      case 's':
    	  moveCueDown();
    	  break;
      case 'a':
          moveCueBack();
          break;
      case 'd':
          moveCueForward();
          break;
      case '+':
    	  raisePower();
    	  break;
      case '-':
    	  lowerPower();
    	  break;
   }

}
void mouse( int button, int state, int x, int y )
{
	if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN)
		   {
			y = ((450 - y)*(50 + 2 * fringeWidth))/450;
			y = y - fringeWidth;
			x = (x*(100 + 2 * fringeWidth))/900;
			x = x - fringeWidth;
			float xdif = abs(balls[2].position.x - x);
			float ydif = abs(balls[2].position.y - y);
			if(xdif < balls[2].radius &&
			   ydif < balls[2].radius)
				{
				 if(balls[4].velocity.x < 1.0 && balls[4].velocity.y <1.0)
				 {
					 balls[3].position.x = x; balls[3].position.y = y;
					 aim();
				 }
				}
		   }
}

/***********************************************************************
 * Racks the board in preparation for the next game
 ***********************************************************************/
void rackBoard()
{
	for(int i = 0; i < MAX_BALLS; i++)
	{
		balls[i].velocity = 0.0;
		balls[i].position = balls[i].oPosition;
		ELASTICITY = 1.0;
	}
	balls[4].hasBeenShot = 0;
	glutPostRedisplay();
}

/***********************************************************************
 * Shoots the ball
 ***********************************************************************/
void shoot()
{
	if(balls[4].velocity.x < 0.6 && balls[4].velocity.y < 0.6)
	{
		balls[4].hasBeenShot = 1;
		balls[3].position = balls[3].oPosition;
		balls[2].position = balls[4].oPosition;
		balls[4].velocity = aimValue*powerValue;
		aimValue.x = 0.0; aimValue.y = 0.0;
		glutPostRedisplay();
	}
}
/***********************************************************************
 * Racks the cue ball
 ***********************************************************************/
void rackCue()
{
	balls[4].hasBeenShot = 0;
	balls[4].velocity = 0.0;
	balls[4].position = balls[4].oPosition;
	glutPostRedisplay();
}
/***********************************************************************
 * Aims the ball
 ***********************************************************************/
vec2 aim()
{
	aimValue.x = balls[2].position.x - balls[3].position.x;
	aimValue.y = balls[2].position.y - balls[3].position.y;
	return aimValue;
}
/***********************************************************************
 * Power modifiers
 ***********************************************************************/
void raisePower(void)
{
	if(powerValue + 1.0 < 15){
	powerValue = powerValue + 1.0;
	std::cout << "The power level is " << powerValue << std::endl;
	}
}
void lowerPower(void)
{
	powerValue = powerValue - 1.0;
	std::cout << "The power level is " << powerValue << std::endl;
}



/***********************************************************************
 * Elasticity Modifiers
 ***********************************************************************/
void elasticityUp()
{
	    if(ELASTICITY == 1.0)
		{
	    	ELASTICITY = 1.5;
	    	std::cout << "Elasticity is raised." << std::endl;
		}
		if(ELASTICITY == 0.5)
		{
			ELASTICITY = 1.0;
			std::cout << "Elasticity is normal." << std::endl;
		}
}
void elasticityDown()
{
	if(ELASTICITY == 1.0)
		{
		ELASTICITY = 0.5;
		std::cout << "Elasticity is lowered." << std::endl;
		}
	if(ELASTICITY == 1.5)
	{
		ELASTICITY = 1.0;
		std::cout << "Elasticity is normal." << std::endl;
	}
}
/***********************************************************************
 * CueBall Size modifiers
 ***********************************************************************/
void ballSizeUp()
{
		std::cout << "Cue ball size and radius are raised." << std::endl;
		balls[4].radius = 5.125;
		balls[4].mass = 10.0;
		balls[4].vao = createCircle(balls[4]);
}
void ballSizeDown()
{
		std::cout << "Cue ball size and radius are normal." << std::endl;
		balls[4].radius = 1.125;
		balls[4].mass = 6;
		balls[4].vao = createCircle(balls[4]);
}

/***********************************************************************
 * CueBall Position modifiers
 ***********************************************************************/
void moveCueUp(void)
{
	if(balls[4].hasBeenShot == 0)
	{
		balls[4].position.y = balls[4].position.y + 1.0;
	}
}
void moveCueDown(void)
{
	if(balls[4].hasBeenShot == 0)
		{
			balls[4].position.y = balls[4].position.y - 1.0;
		}
}
void moveCueForward(void)
{
	if(balls[4].hasBeenShot == 0)
	{
		if(balls[4].position.x + 1.0 <= balls[4].oPosition.x)
		{
			balls[4].position.x = balls[4].position.x + 1.0;
		}
	}
}
void moveCueBack(void)
{
	if(balls[4].hasBeenShot == 0)
		{
			balls[4].position.x = balls[4].position.x - 1.0;
		}
}

/***********************************************************************
 * This computes a simulation step.  Updated ball positions are computed
 * using each ball's velocity.  Then, we check to see if the balls have
 * collided.  If so, we compute the response.  Finally, we see if either
 * ball is leaving the clipping region.  If so, we call placeBalls() to
 * re-start the simulation.
 ***********************************************************************/

void idle(void)
{
	if(currentTick == -1)
	{
		currentTick = GetTickCount();
	}
	int idleTick = GetTickCount();
	int dif = idleTick - currentTick;

   /* Update positions. */

   for (int i = 0; i < MAX_BALLS; ++i){
      balls[i].position += (balls[i].velocity * (dif * .001));
      balls[i].velocity = balls[i].velocity*(1 - friction * (dif * .001));
   }
   if(balls[4].velocity.x < 0.2 && balls[4].velocity.y < 0.2)
   {
	   balls[2].position = balls[4].position;
   }else
   {
	   balls[2].position = -balls[2].oPosition;
   }

   /* Check for collisions and act. */

   for(int j = 0; j < numBalls+numPockets; j++)
   {
	   for(int k = j + 1; k < numBalls+numPockets; k++)
	   {
		   if (balls[j].isIgnored == 0 && balls[k].isIgnored == 0)
		   {
			   if (collision(balls[j], balls[k]))
			   {
					   collisionResponse(balls[j], balls[k]);
			   }
		   }
	   }
   }
   for(int j = 0; j < numBalls; j++)
     {
	   if(balls[j].isIgnored == 0){
  		   if (balls[j].position.x + balls[j].radius > ur.x){
  			   balls[j].velocity.x = -balls[j].velocity.x;
  			   balls[j].position.x = ur.x - balls[j].radius;
  			   }
  		   else if (balls[j].position.y + balls[j].radius > ur.y){
  			   balls[j].velocity.y = balls[j].velocity.y * -1;
  			   balls[j].position.y = ur.y - balls[j].radius;
  			   }
  		   else if (balls[j].position.x - balls[j].radius < ll.x){
  			   balls[j].velocity.x = balls[j].velocity.x * -1;
  			   balls[j].position.x = ll.x + balls[j].radius;
  			   }
  		   else if (balls[j].position.y - balls[j].radius < ll.y){
  			   balls[j].velocity.y = balls[j].velocity.y * -1;
  			   balls[j].position.y = ll.y + balls[j].radius;
  			   }
	   }
     }


   // Re-render the scene. */

   count++;
   if(count == displayThreshold)
   {
	   glutPostRedisplay();
	   count = 0;
   }
   currentTick = idleTick;
}

   
/***********************************************************************
 *  Request double buffer display mode for smooth animation.
 ***********************************************************************/

int main(int argc, char** argv)
{
   srand((unsigned int) time(NULL));

   glutInit(&argc, argv);
#ifdef DOUBLE_BUFFER
   glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
#else
   glutInitDisplayMode(GLUT_RGB);
#endif
   glutInitWindowSize(windowWidth, windowHeight);
   glutInitWindowPosition(100, 100);
   //glutInitContextVersion(3, 2);
   //glutInitContextProfile(GLUT_CORE_PROFILE);
   glutCreateWindow("Colliding balls");

   glewExperimental = GL_TRUE;
   glewInit();

   init();

   glutDisplayFunc(display); 
#ifdef RESHAPE
   glutReshapeFunc(reshape); 
#endif
   glutKeyboardFunc(keyboard);
   glutMouseFunc(mouse);
   glutIdleFunc(idle);
   glutMainLoop();

   return 0;
}
