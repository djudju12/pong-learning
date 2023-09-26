#include <stdio.h>
#include <stdlib.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#define WIDTH 800
#define HEIGTH 800
#define VEL_PAD_Y 15
#define Z_FACTOR 15
#define LINE_LEN 10

typedef struct 
{
   SDL_Rect *p_rect;
   int points;
   SDL_Texture *point_texture;
} Player;

int check_error(int code, char *msg)
{
   if (code != 0)
      printf("ERROR: %s", msg);
   return code;
}

int draw_thick_line(SDL_Renderer *renderer, int thickness)
{
   int center = WIDTH / 2;

   int status;
   for (size_t i = center - thickness; i < center; i++)
   {
      status = SDL_RenderDrawLine(renderer, i, 0, i, HEIGTH);
      if (status != 0)
         return status;
   }

   for (size_t i = center + thickness; i > center; i--)
   {
      status = SDL_RenderDrawLine(renderer, i, 0, i, HEIGTH);
      if (status != 0)
         return status;
   }

   return SDL_RenderDrawLine(renderer, WIDTH / 2, 0, WIDTH / 2, HEIGTH);
}

int draw_circle(SDL_Renderer *renderer, int x, int y, int radius)
{
   SDL_Point points[256];
   int pointCount = 0;
   int offsetx, offsety, d;
   int status;

   offsetx = 0;
   offsety = radius;
   d = radius - 1;
   status = 0;

   while (offsety >= offsetx)
   {
      if (pointCount + 8 > 256)
      {
         status = SDL_RenderDrawPoints(renderer, points, pointCount);
         pointCount = 0;

         if (status < 0)
         {
            status = -1;
            break;
         }
      }

      points[pointCount++] = (SDL_Point){x + offsetx, y + offsety};
      points[pointCount++] = (SDL_Point){x + offsety, y + offsetx};
      points[pointCount++] = (SDL_Point){x - offsetx, y + offsety};
      points[pointCount++] = (SDL_Point){x - offsety, y + offsetx};
      points[pointCount++] = (SDL_Point){x + offsetx, y - offsety};
      points[pointCount++] = (SDL_Point){x + offsety, y - offsetx};
      points[pointCount++] = (SDL_Point){x - offsetx, y - offsety};
      points[pointCount++] = (SDL_Point){x - offsety, y - offsetx};

      if (d >= 2 * offsetx)
      {
         d -= 2 * offsetx + 1;
         offsetx += 1;
      }
      else if (d < 2 * (radius - offsety))
      {
         d += 2 * offsety - 1;
         offsety -= 1;
      }
      else
      {
         d += 2 * (offsety - offsetx - 1);
         offsety -= 1;
         offsetx += 1;
      }
   }

   if (pointCount > 0)
      status = SDL_RenderDrawPoints(renderer, points, pointCount);

   return status;
}

int draw_filled_circle(SDL_Renderer *renderer, int x, int y, int radius)
{
   int status = 0;
   for (size_t i = 0; i <= radius; i++)
   {
      status = draw_circle(renderer, x, y, i);
      if (status != 0)
         break;
   }

   return status;
}

void move_player(SDL_Rect *rect, int amount)
{
   if (amount < 0)
   {
      if (rect->y >= abs(amount))
      {
         rect->y = rect->y + amount;
      }
      return;
   }

   int posy = rect->y + rect->h;
   if (posy <= WIDTH)
   {
      rect->y = rect->y + amount;
   }
}

typedef struct
{
   int x_center, y_center;
   int x_vel, y_vel;
   int radius;
} Ball;

void calc_vel(Ball *ball, SDL_Rect *player)
{
   int pad_center_y = player->y + player->h / 2;
   float y_factor = abs(ball->y_center - pad_center_y) / 100.0;       // if center pad == ball->y_center so y_vel == 0% of FACTOR_Z
   float x_factor = 1 - (abs(ball->y_center - pad_center_y) / 100.0); // if  center pad == ball->y_center so y_vel == 100% of FACTOR_Z

   if (player->x == 0)
   {
      ball->x_vel = (Z_FACTOR * x_factor) + 1;
      if (ball->x_vel == 0)
         ball->x_vel = 1;
   }
   else
   {
      ball->x_vel = -(Z_FACTOR * x_factor + 1);
      if (ball->x_vel == 0)
         ball->x_vel = -1;
   }

   if (ball->y_center > pad_center_y)
      ball->y_vel = Z_FACTOR * y_factor;
   else
      ball->y_vel = -Z_FACTOR * y_factor;
}

void move_ball(Ball *ball, Player *player1, Player *player2)
{
   // point player 1
   if ((WIDTH - ball->x_center) <= ball->radius)
   {
      ball->x_center = WIDTH / 2;
      ball->y_center = HEIGTH / 2;
      player1->points++;
      return;
   }

   // point player 2
   if (ball->x_center <= ball->radius)
   {
      ball->x_center = WIDTH / 2;
      ball->y_center = HEIGTH / 2;
      player2->points++;
      return;
   }

   // if hit player 1
   if (((ball->x_center - player1->p_rect->w) <= ball->radius) &&
       (ball->x_vel < 0))
   {
      if ((ball->y_center + ball->radius >= player1->p_rect->y) &&
          (ball->y_center - ball->radius <= player1->p_rect->y + player1->p_rect->h))
      {
         calc_vel(ball, player1->p_rect);
         return;
      }
   }

   // if hit player 2
   if (((WIDTH - ball->x_center - player2->p_rect->w) <= ball->radius) &&
       (ball->x_vel > 0))
   {
      if ((ball->y_center + ball->radius >= player2->p_rect->y) &&
          (ball->y_center - ball->radius <= player2->p_rect->y + player2->p_rect->h))
      {
         calc_vel(ball, player2->p_rect);
         return;
      }
   }

   // if hit roof
   if (ball->y_center <= ball->radius)
   {
      ball->y_center += ball->radius;
      ball->y_vel = -ball->y_vel;
      return;
   }

   // if hit ceil
   if ((HEIGTH - ball->y_center) <= ball->radius)
   {
      ball->y_center -= ball->radius;
      ball->y_vel = -ball->y_vel;
      return;
   }

   ball->x_center = ball->x_center + ball->x_vel;
   ball->y_center = ball->y_center + ball->y_vel;
}

int mark_points(SDL_Renderer *renderer, TTF_Font *font, Player *player1, Player *player2)
{
   if (player1->point_texture != NULL) SDL_DestroyTexture(player1->point_texture);

   char buffer[33];
   SDL_Rect p1_points = {
      .x = 100/2 + 10,
      .y = 10,
      .w = 50,
      .h = 50
   };

   SDL_Color white = {255, 255, 255};
   sprintf(buffer, "%d", player1->points);
   SDL_Surface *s = TTF_RenderText_Solid(font, buffer, white);
   if (s == NULL){
      printf("SDL_CreateTextureFromSurface on p1\n");
      return -1;
   }

   SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, s);
   if (texture == NULL){
      printf("SDL_CreateTextureFromSurface on p1\n");
      return -1;
   } 

   int status = SDL_RenderCopy(renderer, texture, NULL, &p1_points);
   if (status != 0){
      printf("SDL_RenderCopy on p1\n");
      return -1;
   } 
   SDL_FreeSurface(s);

   player1->point_texture = texture;

   // player 2
   if (player2->point_texture != NULL) SDL_DestroyTexture(player2->point_texture);

   SDL_Rect p2_points = {
      .x = WIDTH-110,
      .y = 10,
      .w = 50,
      .h = 50
   };

   sprintf(buffer, "%d", player2->points);
   SDL_Surface *s2 = TTF_RenderText_Solid(font, buffer, white);
   if (s == NULL){
      printf("SDL_CreateTextureFromSurface on p2\n");
      return -1;
   }

   SDL_Texture *texture2 = SDL_CreateTextureFromSurface(renderer, s2);
     if (texture == NULL){
      printf("SDL_CreateTextureFromSurface on p2\n");
      return -1;
   } 

   status = SDL_RenderCopy(renderer, texture2, NULL, &p2_points);
   if (status != 0){
      printf("SDL_RenderCopy on p2\n");
      return -1;
   } 
   SDL_FreeSurface(s2);

   player2->point_texture = texture2;

   return 0;
}

int main(void)
{
   printf("INFO: initilization of SDL...\n");
   if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
   {
      printf("error initializing SDL: %s\n", SDL_GetError());
      return -1;
   }
   
   SDL_Window *win = SDL_CreateWindow("PONG C", 0, 0, WIDTH, HEIGTH, SDL_WINDOW_BORDERLESS);
   if (!win)
   {
      printf("failed to create window!\n");
      return -1;
   }

   printf("INFO: creating rederer...\n");
   SDL_Renderer *renderer = SDL_CreateRenderer(win, -1, 0);
   if (renderer < 0)
   {
      printf("error initializing SDL Renderer: %s\n", SDL_GetError());
      return -1;
   }


   printf("INFO: initilization of TTF...\n");
   if (TTF_Init() != 0)
   {
      printf("error initializing ttf: %s\n", SDL_GetError());
      return -1;
   }

   char *font_path = "/home/jonathan/hd/programacao/pong-learning/pong-c/ComicMono.ttf";
   printf("INFO: Opening font %s...\n", font_path);
   TTF_Font *font = TTF_OpenFont(font_path, 24);
   if (font == NULL)
   {
      printf("error creating font: %s\n", SDL_GetError());
      return -1;
   }

   SDL_Rect p1_rect = {
       .h = HEIGTH * 0.3,
       .w = WIDTH * 0.03,
       .x = 0,
       .y = HEIGTH / 2};

   Player player1 = {
      .p_rect = &p1_rect,
      .points = 0
   };

   SDL_Rect p2_rect = {
       .h = HEIGTH * 0.3,
       .w = WIDTH * 0.03,
       .x = WIDTH - WIDTH * 0.03,
       .y = HEIGTH / 2};

   Player player2 = {
      .p_rect = &p2_rect,
      .points = 0
   };


   Ball ball = {
       .x_center = WIDTH / 2,
       .y_center = HEIGTH / 2,
       .radius = 10,
       .x_vel = Z_FACTOR / 2,
       .y_vel = Z_FACTOR / 2};

   int err = 0;
   Uint32 lastUpdate = SDL_GetTicks();
   Uint32 now;
   int running = 1;
   while (running)
   {
      SDL_Event event;
      while (SDL_PollEvent(&event))
      {
         switch (event.type)
         {
         case SDL_KEYDOWN:
            switch (event.key.keysym.sym)
            {
            case SDLK_ESCAPE:
               printf("INFO: quiting...\n");
               running = 0;
               break;

            case SDLK_DOWN:
               move_player(player1.p_rect, +VEL_PAD_Y);
               break;

            case SDLK_UP:
               move_player(player1.p_rect, -VEL_PAD_Y);
               break;

            case SDLK_s:
               move_player(player2.p_rect, +VEL_PAD_Y);
               break;

            case SDLK_w:
               move_player(player2.p_rect, -VEL_PAD_Y);
               break;

            default:
               break;
            }

         default:
            break;
         }
      }

      now = SDL_GetTicks();
      if (now > (lastUpdate + 1000 / 60))
      {
         err = check_error(SDL_SetRenderDrawColor(renderer, 18, 18, 18, 255), "SDL_SetRenderDrawColor BACKGROUND");
         if (err != 0)
            break;

         err = check_error(SDL_RenderClear(renderer), "SDL_RenderClear");
         if (err != 0)
            break;

         err = check_error(SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255), "SDL_SetRenderDrawColor WHITE");
         if (err != 0)
            break;

         err = check_error(draw_thick_line(renderer, 3), "drawning line in the center");
         if (err != 0)
            break;

         move_ball(&ball, &player1, &player2);
         err = check_error(draw_filled_circle(renderer, ball.x_center, ball.y_center, ball.radius), "draw ball");
         if (err != 0)
            break;

         err = check_error(mark_points(renderer, font, &player1, &player2), "marking points");
         if (err != 0)
            break;

         err = check_error(SDL_RenderFillRect(renderer, player1.p_rect), "Player1");
         if (err != 0)
            break;

         err = check_error(SDL_RenderFillRect(renderer, player2.p_rect), "Player2");
         if (err != 0)
            break;

         SDL_RenderPresent(renderer);
         lastUpdate = SDL_GetTicks();
      }
   }

   printf("INFO: OK.");
   return 0;
}
