#include <stdio.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL2_gfxPrimitives.h>
#include <SDL2/SDL_image.h>

#define WIDTH 640
#define HEIGTH 640
#define VEL_PAD_Y 10

int checkError(int code, char *msg)
{
    if (code != 0)
        printf("ERROR: %s", msg);
    return code;
}

int drawCircle(SDL_Renderer *renderer, int x, int y, int radius)
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

int drawFilledCircle(SDL_Renderer *renderer, int x, int y, int radius)
{
    int status = 0;
    for (size_t i = 0; i <= radius; i++)
    {
        status = drawCircle(renderer, x, y, i);
        if (status != 0)
            break;
    }

    return status;
}

void movePlayer(SDL_Rect *rect, int amount)
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
    int x_center;
    int y_center;
    int radius;
} Ball;

void moveBall(Ball *ball, int x_vel, int y_vel)
{
    ball->x_center = ball->x_center + x_vel;
    ball->y_center = ball->y_center + y_vel;
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
        .w = WIDTH * 0.05,
        .x = 0,
        .y = HEIGTH / 2};

    SDL_Rect player2 = {
        .h = HEIGTH * 0.3,
        .w = WIDTH * 0.05,
        .x = WIDTH - WIDTH * 0.05,
        .y = HEIGTH / 2};

    int vel_ball_x = 2;
    int vel_ball_y = 3;

    Ball ball = {
        .x_center = WIDTH / 2,
        .y_center = HEIGTH / 2,
        .radius = 10};

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
                    movePlayer(&player1, +VEL_PAD_Y);
                    break;

                case SDLK_UP:
                    movePlayer(&player1, -VEL_PAD_Y);
                    break;

                case SDLK_s:
                    movePlayer(&player2, +VEL_PAD_Y);
                    break;

                case SDLK_w:
                    movePlayer(&player2, -VEL_PAD_Y);
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
            err = checkError(SDL_SetRenderDrawColor(renderer, 18, 18, 18, 255), "SDL_SetRenderDrawColor BACKGROUND");
            if (err != 0)
                break;

            err = checkError(SDL_RenderClear(renderer), "SDL_RenderClear");
            if (err != 0)
                break;

            err = checkError(SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255), "SDL_SetRenderDrawColor WHITE");
            if (err != 0)
                break;

            moveBall(&ball, vel_ball_x, vel_ball_y);
            err = checkError(drawFilledCircle(renderer, ball.x_center, ball.y_center, ball.radius), "draw ball");
            if (err != 0)
                break;

            err = checkError(SDL_RenderFillRect(renderer, &player1), "Player1");
            if (err != 0)
                break;

            err = checkError(SDL_RenderFillRect(renderer, &player2), "Player2");
            if (err != 0)
                break;

            SDL_RenderPresent(renderer);
            lastUpdate = SDL_GetTicks();
        }
    }

    printf("INFO: OK.");
    return 0;
}
