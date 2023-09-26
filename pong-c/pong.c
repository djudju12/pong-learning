#include <stdio.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL2_gfxPrimitives.h>
#include <SDL2/SDL_image.h>

#define WIDTH 800
#define HEIGTH 800
#define VEL_PAD_Y 15
#define Z_FACTOR 8

int check_error(int code, char *msg)
{
    if (code != 0)
        printf("ERROR: %s", msg);
    return code;
}


int draw_thick_line(SDL_Renderer *renderer, int thickness) {
    int center = WIDTH/2;

    int status;
    for (size_t i = center-thickness; i < center; i++)
    {
        status = SDL_RenderDrawLine(renderer, i, 0, i, HEIGTH);
        if (status != 0)  return status;
    }
    
    for (size_t i = center+thickness; i > center; i--)
    {
        status = SDL_RenderDrawLine(renderer, i, 0, i, HEIGTH);
        if (status != 0)  return status;
    }

    return SDL_RenderDrawLine(renderer, WIDTH/2, 0, WIDTH/2, HEIGTH);
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
    int pad_center_y = player->y + player->h/2;
    float y_factor = abs(ball->y_center - pad_center_y)/100.0;       // if center pad == ball->y_center so y_vel == 0% of FACTOR_Z 
    float x_factor = 1 - (abs(ball->y_center - pad_center_y)/100.0); // if  center pad == ball->y_center so y_vel == 100% of FACTOR_Z

    if (player->x == 0) 
    {
        ball->x_vel = (Z_FACTOR*x_factor) + 1;
        if (ball->x_vel == 0) ball->x_vel = 1;
    } 
    else 
    {
        ball->x_vel = -(Z_FACTOR*x_factor + 1);
        if (ball->x_vel == 0) ball->x_vel = -1;
    }

    if (ball->y_center > pad_center_y) ball->y_vel = Z_FACTOR*y_factor;
    else ball->y_vel = -Z_FACTOR*y_factor;

}

void move_ball(Ball *ball, SDL_Rect *player1, SDL_Rect *player2)
{
    // point player 1
    if ((WIDTH - ball->x_center) <= ball->radius)
    {
        ball->x_center = WIDTH / 2;
        ball->y_center = HEIGTH / 2;
        return;
    }

    // point player 2
    if (ball->x_center <= ball->radius)
    {
        ball->x_center = WIDTH / 2;
        ball->y_center = HEIGTH / 2;
        return;
    }

    // if hit player 1
    if (((ball->x_center - player1->w) <= ball->radius) && 
         (ball->x_vel < 0))
    {
        if ((ball->y_center + ball->radius >= player1->y) && 
            (ball->y_center - ball->radius <= player1->y + player1->h)) 
        {
            calc_vel(ball, player1);
            return;
        }
    }

    // if hit player 2
    if (((WIDTH - ball->x_center - player2->w) <= ball->radius) && 
        (ball->x_vel > 0))
    {
        if ((ball->y_center + ball->radius >= player2->y) && 
            (ball->y_center - ball->radius <= player2->y + player2->h)) 
        {
            calc_vel(ball, player2);
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

int main(void)
{
    printf("INFO: initilization of SDL...\n");
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
    {
        printf("error initializing SDL: %s\n", SDL_GetError());
        return -1;
    }

    // SDL_Window *win = SDL_CreateWindow("PONG C", 0, 0, WIDTH, HEIGTH, SDL_WINDOW_FULLSCREEN_DESKTOP);
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

    SDL_Rect player1 = {
        .h = HEIGTH * 0.3,
        .w = WIDTH * 0.03,
        .x = 0,
        .y = HEIGTH / 2};

    SDL_Rect player2 = {
        .h = HEIGTH * 0.3,
        .w = WIDTH * 0.03,
        .x = WIDTH - WIDTH * 0.03,
        .y = HEIGTH / 2};

    Ball ball = {
        .x_center = WIDTH / 2,
        .y_center = HEIGTH / 2,
        .radius = 10,
        .x_vel = Z_FACTOR/2,
        .y_vel = Z_FACTOR/2};

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
                    move_player(&player1, +VEL_PAD_Y);
                    break;

                case SDLK_UP:
                    move_player(&player1, -VEL_PAD_Y);
                    break;

                case SDLK_s:
                    move_player(&player2, +VEL_PAD_Y);
                    break;

                case SDLK_w:
                    move_player(&player2, -VEL_PAD_Y);
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

            err = check_error(SDL_RenderFillRect(renderer, &player1), "Player1");
            if (err != 0)
                break;

            err = check_error(SDL_RenderFillRect(renderer, &player2), "Player2");
            if (err != 0)
                break;

            SDL_RenderPresent(renderer);
            lastUpdate = SDL_GetTicks();
        }
    }

    printf("INFO: OK.");
    return 0;
}
