#include <stdio.h>
#include <stdlib.h>
#include <GL/glut.h>
#include <math.h>
#include "surface_textures.ppm"
#include "sky.ppm"
#include "title_new_scaled.ppm"
#include "won.ppm"
#include "lost.ppm"
#include "sprites.ppm"

#define WINDOW_WIDTH 960
#define WINDOW_HEIGHT 640
#define LINE_WIDTH 8

#define MAP_WIDTH	8
#define MAP_HEIGHT	8
#define BLOCK_SIZE	64

#define SPAWN_X		96
#define SPAWN_Y		96
#define SPAWN_ANGLE 315
#define EXIT_X		4
#define EXIT_Y		6

#define CAM_FOV		70
#define MOV_SPEED	0.15
#define ROT_SPEED	0.25

#define V_MARGIN 2
#define H_MARGIN 2

// game states
#define INIT		0
#define TITLE		1
#define PLAY		2
#define WIN			3
#define LOSS		4
// textures
#define COBBLE_1		1
#define BRICKS			2
#define COBBLE_2		3
#define DOOR			4
#define COBBLE_1_LIGHT	5
#define EXIT			6
#define COBBLE_2_SHAD	7
#define SPAWN			8
#define COBBLE_1_SHAD	9
// sprites
#define KEY		0
#define LIGHT	1
#define ENEMY	2


struct point {
	float x;
	float y;
};

struct vector {
	float x;
	float y;
	float dx;
	float dy;
	float angle;
};

// structure for holding the state of the keys (pressed or not)
struct controls {
	int a, d, w, s;
};

struct sprite {
	int type;		// static / key / enemy
	int state;		// on / off
	int texture;
	float x, y, z;	// position
};

struct vector player;
struct controls keys;
struct sprite sprt[4];

float frame1, frame2, fps;
int depth[WINDOW_WIDTH / LINE_WIDTH];

int gameState = 0, timer = 0;
float fade = 0;


int map_walls[] = {
	2,2,2,2,2,2,2,2,
	2,0,0,0,0,0,0,2,
	2,0,0,2,0,0,0,2,
	2,0,0,0,0,0,0,2,
	2,0,0,1,1,0,0,2,
	2,0,1,1,2,2,4,2,
	2,0,0,1,0,0,0,2,
	2,2,2,2,6,2,2,2,
};

int map_ceiling[] = {
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,2,0,0,0,0,
	0,0,0,2,2,2,2,0,
	0,2,2,2,2,0,0,0,
	0,2,2,2,2,0,0,0,
	0,0,0,0,0,0,0,0,
};

int map_floor[] = {
	1,1,1,1,1,1,1,1,
	1,2,2,2,2,2,2,1,
	1,2,2,2,2,2,2,1,
	1,2,2,2,2,2,2,1,
	1,2,2,2,2,4,0,1,
	1,2,2,2,2,2,2,1,
	1,2,7,2,2,2,2,1,
	1,1,1,1,1,1,1,1,
};

float normalize_angle(float a) {
	// prevent the angle from overflowing
	if(a > 360)	a -= 360;
	// prevent the angle from underflowing
	if(a < 0)	a += 360;
	return a;
}

float deg2rad(float deg) {
	return deg * M_PI / 180.0;
}

float distance(struct vector a, struct point b)	{
	return (sqrt((b.x - a.x) * (b.x - a.x) + (b.y - a.y) * (b.y - a.y)));
}

void butt_pressed(unsigned char key, int x, int y) {
	if(key == 'a')	keys.a = 1;
	if(key == 'd')	keys.d = 1;
	if(key == 'w')	keys.w = 1;
	if(key == 's')	keys.s = 1;
	if(key == 'e')
	{
		// calculate player's position and orientation
		int x0 = 0;
		if(player.dx < 0)	x0 = -25;
		else				x0 =  25;
		int y0 = 0;
		if(player.dy < 0)	y0 = -25;
		else				y0 =  25;
		int ipx = player.x / 64.0;
		int ipx_add_x0 = (player.x + x0) / 64.0;
		int ipy = player.y / 64.0;
		int ipy_add_y0 = (player.y + y0) / 64.0;
		
		// if you are facing a door and have the key
		if(map_walls[ipy_add_y0 * MAP_WIDTH + ipx_add_x0] == DOOR)
			if(sprt[KEY].state == 0)
				// then replace the door with an empty space
				map_walls[ipy_add_y0 * MAP_WIDTH + ipx_add_x0] = 0;
		// if you are facing the exit, then you won
		if(map_walls[ipy_add_y0 * MAP_WIDTH + ipx_add_x0] == EXIT)
		{
			timer = 0;
			fade = 0;
			gameState = WIN;
		}
				
	}
	glutPostRedisplay();
}

void butt_released(unsigned char key, int x, int y) {
	if(key == 'a')	keys.a = 0;
	if(key == 'd')	keys.d = 0;
	if(key == 'w')	keys.w = 0;
	if(key == 's')	keys.s = 0;
	glutPostRedisplay();
}


void drawMap2D() {
	int x, y, x0, y0;
	for(y = 0; y < MAP_HEIGHT; y++)
	{
		for(x = 0; x < MAP_WIDTH; x++)
		{
			if(map_walls[y * MAP_WIDTH + x] > 0)
				glColor3f(1, 1, 1);
			else
				glColor3f(0, 0, 0);
			// scale the map by a factor of 64
			x0 = x * BLOCK_SIZE + H_MARGIN;
			y0 = y * BLOCK_SIZE + V_MARGIN;
			// draw a quadrilateral (square in our case)
			glBegin(GL_QUADS);
			// defined by these four points
			glVertex2i(x0					, y0				);
			glVertex2i(x0					, y0 + BLOCK_SIZE-2	);
			glVertex2i(x0 + BLOCK_SIZE-2	, y0 + BLOCK_SIZE-2	);
			glVertex2i(x0 + BLOCK_SIZE-2	, y0				);
			// the -2 leaves a grey border around the sides of the squares
			glEnd();
		}
	}	
}

void drawPlayer() {
	// draw a point at player's position
	glColor3f(1, 1, 0);
	glPointSize(LINE_WIDTH);
	glBegin(GL_POINTS);
	glVertex2i(player.x, player.y);
	glEnd();
	// draw a short line in the direction in which player is facing
	glLineWidth(3);
	glBegin(GL_LINES);
	glVertex2i(player.x, player.y);
	glVertex2i(player.x + player.dx*20, player.y + player.dy*20);
	glEnd();
}

void drawRays2D() {
	struct vector ray;
	struct point h_intersect, v_intersect, next_intersect;
	int r, i, mx, my, dof;
	float dist, disH = 1000000;
		
	ray.angle = normalize_angle(player.angle + CAM_FOV / 2.0);
	
	h_intersect.x = v_intersect.x = player.x;
	h_intersect.y = v_intersect.y = player.y;
		
	//remember the coordinate system
	// 0---> x
	// |
	// v
	// y
	
	//for(r = 1; r < 15; r++)
	// iterate for every vertical line
	for(r = 0; r < WINDOW_WIDTH / LINE_WIDTH; r++)
	{
		// vertical and horizontal map texture numbers
		int vmt = 0, hmt = 0;
		// find where the ray intersects the horizontal gridline
		dof = 0;
		float aTan = 1 / tan(deg2rad(ray.angle));		
		// if looking up
		if(sin(deg2rad(ray.angle)) > 0.001)
		{
			// the ray's y coordinate can be found by rounding
			// the player's y coordinate down to the nearest 64th value
			ray.y = (((int)player.y >> 6) << 6) -0.0001;
			// the ray's x coordinate can then be triangulated as
			ray.x = (player.y - ray.y) * aTan + player.x;
			// the coordinates of the next intersecetion
			// are found by a simple offset
			next_intersect.y = -64;
			next_intersect.x = -next_intersect.y * aTan;
		}
		// if looking down
		else if(sin(deg2rad(ray.angle)) < -0.001)
		{
			// the ray's y coordinate can be found by rounding
			// the player's y coordinate up to the nearest 64th value
			ray.y = (((int)player.y >> 6) << 6) +64;
			// the ray's x coordinate can then be triangulated as
			ray.x = (player.y - ray.y) * aTan + player.x;
			// the coordinates of the next intersecetion
			// are found by a simple offset
			next_intersect.y = 64;
			next_intersect.x = -next_intersect.y * aTan;
		}
		// if looking straight horizontally
		else
		{
			ray.x = player.x;
			ray.y = player.y;
			// end the loop
			dof = 8;
		}
		while(dof < 8)
		{
			// scale down the ray's coordinates by 64 so that they can be used for indexing
			mx = (int)(ray.x) >> 6;
			my = (int)(ray.y) >> 6;
			// if the ray intersects the grid outside the map in negative direction
			// limit the index to zero
			if(mx < 0)	mx = 0;
			if(my < 0)	my = 0;
			// and use them to address the map array
			i = my * MAP_WIDTH + mx;
			// are we are addressing inside the map array?
			if((i >= 0) && (i < MAP_WIDTH * MAP_HEIGHT))
			{
				// if a wall is hit
				if(map_walls[i] > 0)
				{
					hmt = map_walls[i] - 1;
					h_intersect.x = ray.x;
					h_intersect.y = ray.y;
					disH = distance(player, h_intersect);
					//disH = cos(deg2rad(ray.angle)) * (ray.x - player.x) - sin(deg2rad(ray.angle)) * (ray.y - player.y);
					dof = 8;				
				}
				/// if not, further extend the ray
				else
				{
					ray.x += next_intersect.x;
					ray.y += next_intersect.y;
					dof += 1;
				}
			}
			// do not calculate outside of the array
			else
			{
				//ray.x += next_intersect.x;
				//ray.y += next_intersect.y;
				dof = 8;				
			}
		}

		/*
		// display horizontal ray for debugging			
		glColor3f(0,1,0);
		glLineWidth(5);
		glBegin(GL_LINES);
		glVertex2i(player.x, player.y);
		glVertex2i(ray.x, ray.y);
		glEnd();
		*/

		// find where the ray intersects the vertical gridline
		dof = 0;
		float disV = 1000000;
		float Tan = 0;
		//if(ray.angle != 0 && ray.angle != 360)
			Tan = tan(deg2rad(ray.angle));		
		//printf("Tan = %.9g\n", Tan);						
		// if looking left
		if(cos(deg2rad(ray.angle)) < -0.001)
		{
			// the ray's x coordinate can be found by rounding
			// the player's x coordinate down to the nearest 64th value
			ray.x = (((int)player.x >> 6) << 6) -0.0001;
			// the ray's y coordinate can then be triangulated as
			ray.y = (player.x - ray.x) * Tan + player.y;
			// the coordinates of the next intersecetion
			// are found by a simple offset
			next_intersect.x = -64;
			next_intersect.y = -next_intersect.x * Tan;
		}
		// if looking right
		else if(cos(deg2rad(ray.angle)) > 0.001)
		{
			// the ray's x coordinate can be found by rounding
			// the player's x coordinate up to the nearest 64th value
			ray.x = (((int)player.x >> 6) << 6) + 64;
			// the ray's y coordinate can then be triangulated as
			ray.y = (player.x - ray.x) * Tan + player.y;
			// the coordinates of the next intersecetion
			// are found by a simple offset
			next_intersect.x = 64;
			next_intersect.y = -next_intersect.x * Tan;
		}
		// if looking straight vertically
		else
		{
			ray.x = player.x;
			ray.y = player.y;
			// end the loop
			dof = 8;
		}
		while(dof < 8)
		{
			// scale down the ray's coordinates by 64 so that they can be used for indexing
			mx = (int)(ray.x) >> 6;
			my = (int)(ray.y) >> 6;
			// if the ray intersects the grid outside the map in negative direction
			// limit the index to zero
			if(mx < 0)	mx = 0;
			if(my < 0)	my = 0;
			i = my * MAP_WIDTH + mx;
			// are we are addressing inside the map array?
			if((i >= 0) && (i < MAP_WIDTH * MAP_HEIGHT))
			{
				// if a wall is hit
				if(map_walls[i] > 0)
				{
					vmt = map_walls[i] - 1;
					v_intersect.x = ray.x;
					v_intersect.y = ray.y;
					disV = distance(player, v_intersect);
					//disV = cos(deg2rad(ray.angle)) * (ray.x - player.x) - sin(deg2rad(ray.angle)) * (ray.y - player.y);
					dof = 8;				
				}
				// if not, further extend the ray
				else
				{
					ray.x += next_intersect.x;
					ray.y += next_intersect.y;
					dof += 1;
				}
			}
			// do not calculate outside of the array
			else
				dof = 8;				
		}
			
		float shade;
			
		if(disV < disH)
		{
			hmt = vmt;
			ray.x = v_intersect.x;
			ray.y = v_intersect.y;
			dist = disV;
			// make the vertical wall darker
			shade = 0.5;		
		}
		else
		{
			ray.x = h_intersect.x;
			ray.y = h_intersect.y;
			dist = disH;
			// make the horizontal wall lighter
			shade = 1;
		}								
		
		
		// draw the shortest ray for debugging
		/*
		//glColor3f(1,0,0);
		glLineWidth(1);
		glBegin(GL_LINES);
		glVertex2i(player.x + H_MARGIN, player.y + V_MARGIN);
		glVertex2i(ray.x + H_MARGIN, ray.y + V_MARGIN);
		glEnd();
		*/
		
		// save the ray's depth for later sprite calculations
		depth[r] = dist;
		
		// draw walls
		
		// remove the fisheye effect
		float angle_player_ray = normalize_angle(player.angle - ray.angle);
		dist = dist * cos(deg2rad(angle_player_ray));
		// -------------------------
		
		int lineH = (BLOCK_SIZE * WINDOW_HEIGHT) / dist;
		float ty_step = 32.0 / (float)lineH;
		float texture_offset = 0;
		
		if(lineH > WINDOW_HEIGHT)
		{
			texture_offset = (lineH - WINDOW_HEIGHT) / 2.0;
			lineH = WINDOW_HEIGHT;
		}
		float line_offset = (WINDOW_HEIGHT - lineH) / 2;
			
		int y;
		float ty = texture_offset * ty_step;
		float tx;
		if(shade == 1)
		{
			tx = (int)(ray.x / 2.0) % 32;
			// mirror the textures if looking down
			if(ray.angle > 180)
				tx = 31 - tx;		
		}
		else
		{
			tx = (int)(ray.y / 2.0) % 32;
			// mirror the textures if looking left
			if(ray.angle > 90 && ray.angle < 270)
				tx = 31 - tx;		
		}
		
		
		
		// draw walls
		for(y = 0; y < lineH; y++) {
			// different types of wall get different colors

			int pixel	= ((int)ty * 32 + (int)tx) * 3 + (hmt * 32 * 32 * 3);
			int red		= textures[pixel + 0] * shade;
			int green	= textures[pixel + 1] * shade;
			int blue	= textures[pixel + 2] * shade;
			glLineWidth(LINE_WIDTH);
			glColor3ub(red, green, blue);
			glBegin(GL_POINTS);
			glVertex2i(LINE_WIDTH/2 + r * LINE_WIDTH, line_offset + y);
			glEnd();
			ty += ty_step;
		}
		
		// +1 in the for loop prevents the ceiling glitch
		// - 3 in the for loop ensures the floor ends within the bounds of the camera
		for(y = line_offset + lineH + 1; y < WINDOW_HEIGHT; y++) 
		{
			// draw floor	
			float dy = y - (WINDOW_HEIGHT / 2.0);
			float ray_fix = cos(deg2rad(angle_player_ray));
			tx = player.x / 2 + cos(deg2rad(ray.angle)) * 158*2 * 32 / dy / ray_fix;
			ty = player.y / 2 - sin(deg2rad(ray.angle)) * 158*2 * 32 / dy / ray_fix;
			int i = map_floor[(int)(ty / 32.0) * MAP_WIDTH + (int)(tx / 32.0)] * 32 * 32;
			// multiplication by 3 because of RGB
			// bitwise and by 31 selects the exact texture
			int pixel	= (((int)ty & 31) * 32 + ((int)(tx) & 31)) * 3 + i * 3;
			int red		= textures[pixel + 0] * 0.7;
			int green	= textures[pixel + 1] * 0.7;
			int blue	= textures[pixel + 2] * 0.7;
			glLineWidth(LINE_WIDTH);
			glColor3ub(red, green, blue);
			glBegin(GL_POINTS);
			glVertex2i(LINE_WIDTH/2 + r * LINE_WIDTH, y);
			glEnd();			
			
			// draw ceiling
			i = map_ceiling[(int)(ty / 32.0) * MAP_WIDTH + (int)(tx / 32.0)] * 32 * 32;
			// if the block has a texture	
			if (i > 0)
			{
				// multiplication by 3 because of RGB
				// bitwise AND by 31 selects the exact texture
				pixel	= (((int)ty & 31) * 32 + ((int)(tx) & 31)) * 3 + i * 3;
				red		= textures[pixel + 0];
				green	= textures[pixel + 1];
				blue	= textures[pixel + 2];
				glLineWidth(LINE_WIDTH);
				glColor3ub(red, green, blue);
				glBegin(GL_POINTS);
				// subracting half the LINE_WIDTH aligns the ceiling to the top of the wall
				glVertex2i(LINE_WIDTH/2 + r * LINE_WIDTH, WINDOW_HEIGHT - y - LINE_WIDTH/2);
				glEnd();				
			}
			
		}
		
		
		// decrement the angle while keeping it inside the unit circle
		float angle_step = (float)CAM_FOV/((float)WINDOW_WIDTH/LINE_WIDTH);
		ray.angle = normalize_angle(ray.angle - angle_step);
	}
}

void drawSimpleSky() {
	// draw a simple sky
	glColor3f(0,1,1); glBegin(GL_QUADS);
	glVertex2i(H_MARGIN					, V_MARGIN						);
	glVertex2i(H_MARGIN + WINDOW_WIDTH	, V_MARGIN						);
	glVertex2i(H_MARGIN + WINDOW_WIDTH	, V_MARGIN + WINDOW_HEIGHT / 2	);
	glVertex2i(H_MARGIN					, V_MARGIN + WINDOW_HEIGHT / 2	);
	glEnd();
	// draw a simple ground	
	glColor3f(0,0,1); glBegin(GL_QUADS);
	glVertex2i(H_MARGIN					, V_MARGIN + WINDOW_HEIGHT / 2	);
	glVertex2i(H_MARGIN + WINDOW_WIDTH	, V_MARGIN + WINDOW_HEIGHT / 2	);
	glVertex2i(H_MARGIN + WINDOW_WIDTH	, V_MARGIN + WINDOW_HEIGHT		);
	glVertex2i(H_MARGIN					, V_MARGIN + WINDOW_HEIGHT		);
	glEnd();	
	
}


void drawSky() {
	
	int x, y;
	for(y = 0; y < WINDOW_HEIGHT / LINE_WIDTH; y++)
	{
		for(x = 0; x < WINDOW_WIDTH / LINE_WIDTH; x++)
		{
			// rotate the sky around the player
			int x0 = (int) player.angle*2 - x;
			if(x0 < 0)	x0 += 120;
			// modulo kepps the index within the bounds of the texture
			x0 %= 120;
			int pixel	= (y * 120 + x0) * 3;
			int red		= sky[pixel + 0];
			int green	= sky[pixel + 1];
			int blue	= sky[pixel + 2];
			glLineWidth(LINE_WIDTH);
			glColor3ub(red, green, blue);
			glBegin(GL_POINTS);
			glVertex2i(LINE_WIDTH/2 + x * LINE_WIDTH, y * LINE_WIDTH);
			glEnd();
		}
	}	
	
}              

void drawSprites() {
	
	int i, x, y;
	
	// enemy-wall collisions
	// get enemy coordinates
	int spx = (int)sprt[ENEMY].x >> 6;
	int spy = (int)sprt[ENEMY].y >> 6;
	// calculate a buffer around the position of the enemy
	int spx_add = ((int)sprt[ENEMY].x + 15) >> 6;
	int spy_add = ((int)sprt[ENEMY].y + 15) >> 6;
	int spx_sub = ((int)sprt[ENEMY].x - 15) >> 6;
	int spy_sub = ((int)sprt[ENEMY].y - 15) >> 6;
	// make the enemy follow you while using the buffer to avoid collisions with walls
	if(sprt[ENEMY].x > player.x && map_walls[spy*8 + spx_sub] == 0)
		sprt[ENEMY].x -= 0.03 * fps;
	if(sprt[ENEMY].x < player.x && map_walls[spy*8 + spx_add] == 0)
		sprt[ENEMY].x += 0.03 * fps;
	if(sprt[ENEMY].y > player.y && map_walls[spy_sub*8 + spx] == 0)
		sprt[ENEMY].y -= 0.03 * fps;
	if(sprt[ENEMY].y < player.y && map_walls[spy_add*8 + spx] == 0)
		sprt[ENEMY].y += 0.03 * fps;
	
	for(i = 0; i < 3; i++)
	{
		// calculate the position of the sprite relative to the player
		float sx = sprt[i].x - player.x;
		float sy = sprt[i].y - player.y;
		float sz = sprt[i].z;
	
		// the sprite needs to be rotated around the player	
		float CS = cos(deg2rad(player.angle));
		float SN = sin(deg2rad(player.angle));
		float a = sy * CS + sx * SN;
		float b = sx * CS - sy * SN;
		sx = a;
		sy = b;
		// convert to screen coordinates
		sx = (sx * 108 / sy) + (WINDOW_WIDTH / LINE_WIDTH / 2);
		sy = (sz * 108 / sy) + (WINDOW_HEIGHT / LINE_WIDTH / 2);

		int scale = 32 * 80/b;
		if(scale < 0)	scale = 0;
		if(scale > 120)	scale = 120;
		
		// texture
		float t_x = 0, t_y = 32, t_x_step = 31.5 / (float)scale, t_y_step = 32.0 / (float)scale;
		
		for(x = sx - scale/2; x < sx + scale/2; x++)
		{
			t_y = 31;
			for(y = 0; y < scale; y++)
			{
				// draw the sprite only when it is active, is within the screen, and is not covered by a wall
				if(sprt[i].state == 1  &&  sx > 0  &&  sx < WINDOW_WIDTH/LINE_WIDTH  &&  b < depth[(int)sx])
				{	
					int pixel	= ((int)t_y * 32 + (int)t_x) * 3 + sprt[i].texture*32*32*3;
					int red		= sprite[pixel + 0];
					int green	= sprite[pixel + 1];
					int blue	= sprite[pixel + 2];
					// do not draw the transparent purple color
					if(red != 255, green != 0, blue != 255)
					{
						glPointSize(LINE_WIDTH);
						glColor3ub(red, green, blue);
						glBegin(GL_POINTS);
						glVertex2i(LINE_WIDTH/2 + x * LINE_WIDTH, sy * LINE_WIDTH - y * LINE_WIDTH);
						glEnd();						
					}
					t_y -= t_y_step;
					if(t_y < 0)
						t_y = 0;	
				}			
				
			}
			t_x += t_x_step;
		}
		
	}
}

void displayPic(int n) {
	
	int x, y;
	int *picture;
	
	if(n == 1)	picture = title;
	if(n == 2)	picture = won;
	if(n == 3)	picture = lost;
	
	for(y = 0; y < WINDOW_HEIGHT / LINE_WIDTH; y++)
	{
		for(x = 0; x < WINDOW_WIDTH / LINE_WIDTH; x++)
		{
			int pixel	= (y * 120 + x) * 3;
			int red		= picture[pixel + 0] * fade;
			int green	= picture[pixel + 1] * fade;
			int blue	= picture[pixel + 2] * fade;
			glPointSize(LINE_WIDTH);
			glColor3ub(red, green, blue);
			glBegin(GL_POINTS);
			glVertex2i(LINE_WIDTH/2 + x * LINE_WIDTH, LINE_WIDTH/2 + y * LINE_WIDTH);
			glEnd();
		}
	}
	// make the picture fade		
	if(fade < 1)
		fade += 0.001 * fps;
	if(fade > 1)
		fade = 1;
}

void init() {
	// set the background of the window to dark grey
	glClearColor(0.3, 0.3, 0.3, 0);
	// initialize player's starting position and direction
	player.x = SPAWN_X; player.y = SPAWN_Y; player.angle = SPAWN_ANGLE;
	// calculate the player's vector at start
	player.dx = cos(deg2rad(player.angle));
	player.dy = -sin(deg2rad(player.angle));
	// replace the doors
	map_walls[46] = DOOR;
	// intiliaze the sprites
	sprt[KEY].type = KEY;				sprt[KEY].state = 1;				sprt[KEY].texture = KEY;
	sprt[KEY].x = 2.5 * BLOCK_SIZE;		sprt[KEY].y = 6.5 * BLOCK_SIZE;		sprt[KEY].z = 20;
	sprt[LIGHT].type = LIGHT;			sprt[LIGHT].state = 1;				sprt[LIGHT].texture = LIGHT;
	sprt[LIGHT].x = 5.5 * BLOCK_SIZE;	sprt[LIGHT].y = 4.5 * BLOCK_SIZE;	sprt[LIGHT].z = 0;
	sprt[ENEMY].type = ENEMY;			sprt[ENEMY].state = 1;				sprt[ENEMY].texture = ENEMY;
	sprt[ENEMY].x = 6 * BLOCK_SIZE;		sprt[ENEMY].y = 1.5 * BLOCK_SIZE;	sprt[ENEMY].z = 20;
	
}

void calculate_movement() {
	
	// check for rotational movement
	if(keys.a == 1)
	{
		player.angle += ROT_SPEED * fps;
		player.angle = normalize_angle(player.angle);
		// recalculate the player's vector
		player.dx = cos(deg2rad(player.angle));
		player.dy = -sin(deg2rad(player.angle));
	}
	if(keys.d == 1)
	{
		player.angle -= ROT_SPEED * fps;
		player.angle = normalize_angle(player.angle);
		// recalculate the player's vector
		player.dx = cos(deg2rad(player.angle));
		player.dy = -sin(deg2rad(player.angle));
	}
	
	// prepare collision buffers
	struct point coll_buff;
	if(player.dx < 0)	coll_buff.x = -20;
	else				coll_buff.x =  20;
	if(player.dy < 0)	coll_buff.y = -20;
	else				coll_buff.y =  20;	
	int ipx = player.x / 64.0;
	int ipx_add_x0 = (player.x + coll_buff.x) / 64.0;
	int ipx_sub_x0 = (player.x - coll_buff.x) / 64.0;
	int ipy = player.y / 64.0;
	int ipy_add_y0 = (player.y + coll_buff.y) / 64.0;
	int ipy_sub_y0 = (player.y - coll_buff.y) / 64.0;
	
	// check for frontal movement, watch out for collisions
	if(keys.w == 1)
	{
		if(map_walls[ipy * MAP_WIDTH		+	ipx_add_x0	] == 0)	player.x += player.dx * MOV_SPEED * fps;
		if(map_walls[ipy_add_y0 * MAP_WIDTH	+	ipx			] == 0)	player.y += player.dy * MOV_SPEED * fps;
	}
	if(keys.s == 1)
	{
		if(map_walls[ipy * MAP_WIDTH		+	ipx_sub_x0	] == 0)	player.x -= player.dx * MOV_SPEED * fps;
		if(map_walls[ipy_sub_y0 * MAP_WIDTH	+	ipx			] == 0)	player.y -= player.dy * MOV_SPEED * fps;
	}

	// if near a key, pick it up
	if(player.x < sprt[KEY].x + 30   &&  player.x > sprt[KEY].x - 30)
		if(player.y < sprt[KEY].y + 30  &&  player.y > sprt[KEY].y - 30)
			sprt[KEY].state = 0;
			
	// if near an enemy, die
	if(player.x < sprt[ENEMY].x + 30   &&  player.x > sprt[ENEMY].x - 30)
		if(player.y < sprt[ENEMY].y + 30  &&  player.y > sprt[ENEMY].y - 30)
			gameState = LOSS;
	
}

void display() {

	// calculate the frame rate
	frame2 = glutGet(GLUT_ELAPSED_TIME);
	fps = frame2 - frame1;
	frame1 = glutGet(GLUT_ELAPSED_TIME);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// display an image according to the state of the game
	if(gameState == INIT)
	{
		init();
		timer = 0;
		fade = 0;
		gameState = TITLE;
	}
	if(gameState == TITLE)
	{
		// display title screen
		displayPic(1);
		timer += 1 * fps;
		// fade out the screen
		if(timer > 1500)
		{
			gameState = 2;
			timer = 0;
			fade = 0;
		}
	}
	if(gameState == PLAY)
	{
		calculate_movement();	
		
		// top down view for debugging
		//drawMap2D();
		//drawPlayer();
		//printf("player angle = %.9g rad (%.9g degrees)\n", deg2rad(player.angle), player.angle);
		
		drawSky();
		drawRays2D();
		drawSprites();
		
	}
	if(gameState == WIN)
	{
		// display won screen
		displayPic(2);
		timer += 1 * fps;
		if(timer > 2000)
		{
			fade = 0;
			timer = 0;
			gameState = 0;
		}
	}
	if(gameState == LOSS)
	{
		// display lost screen
		displayPic(3);
		timer += 1 * fps;
		if(timer > 2000)
		{
			fade = 0;
			timer = 0;
			gameState = 0;
		}	
	}

	glutPostRedisplay();
	glutSwapBuffers();
}

void resize(int w, int h) {
	glutReshapeWindow(WINDOW_WIDTH, WINDOW_HEIGHT);
}

int main(int argc, char** argv)
{ 
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
	glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
	// make the window pop up in the center of the screen
	glutInitWindowPosition((glutGet(GLUT_SCREEN_WIDTH) - WINDOW_WIDTH) / 2, (glutGet(GLUT_SCREEN_HEIGHT) - WINDOW_HEIGHT) / 2);
	glutCreateWindow("raycaster demo");
	// setup an ortographic projection matrix
	gluOrtho2D(0, WINDOW_WIDTH, WINDOW_HEIGHT, 0);	
	init();
	glutDisplayFunc(display);
	// scale the window back to prevent artifacts (if it was resized by the user)
	glutReshapeFunc(resize);
	glutKeyboardFunc(butt_pressed);
	glutKeyboardUpFunc(butt_released);
	glutMainLoop();
	return 0;
}
