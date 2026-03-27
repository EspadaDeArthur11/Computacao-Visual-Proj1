// Copyright (c) 2026 Andre Kishimoto - https://kishimoto.com.br/
// SPDX-License-Identifier: Apache-2.0

//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>

//------------------------------------------------------------------------------
// Custom types, structs, constants, etc.
//------------------------------------------------------------------------------
static const char *WINDOW_TITLE = "Imagem";
static const char *CHILD_WINDOW_TITLE = "Histograma";
static char *IMAGE_FILENAME = "";

enum constants
{
    DEFAULT_WINDOW_WIDTH = 640,
    DEFAULT_WINDOW_HEIGHT = 480,
    PIXEL_DEPTH = 256,
};

typedef struct MyWindow MyWindow;
struct MyWindow
{
    SDL_Window *window;
    SDL_Renderer *renderer;
};

typedef struct MyImage MyImage;
struct MyImage
{
    SDL_Surface *surface;
    SDL_Texture *texture;
    SDL_FRect rect;
};

//------------------------------------------------------------------------------
// Globals
//------------------------------------------------------------------------------
static MyWindow g_window = { .window = NULL, .renderer = NULL };
static MyWindow g_childWindow = { .window = NULL, .renderer = NULL };
static MyImage g_image = {
    .surface = NULL,
    .texture = NULL,
    .rect = { .x = 0.0f, .y = 0.0f, .w = 0.0f, .h = 0.0f }
};
int hist[PIXEL_DEPTH];  // mapeia um nível de intensidade para a sua frequência na imagem
int max_hist = 0;       // valor de frequência máximo 
double mean_hist = 0.0;
double std_dev_hist = 0.0;


//------------------------------------------------------------------------------
// Function declaration
//------------------------------------------------------------------------------
static bool MyWindow_initialize(MyWindow *window, const char *title, int width, int height, SDL_WindowFlags window_flags);
static void MyWindow_destroy(MyWindow *window);
static void MyImage_destroy(MyImage *image);

/**
 * Carrega a imagem indicada no parâmetro `filename` e a converte para o formato
 * RGBA32, eliminando dependência do formato original da imagem. A imagem
 * carregada é armazenada em output_image.
 */
static void load_rgba32(const char *filename, SDL_Renderer *renderer, MyImage *output_image);

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
bool MyWindow_initialize(MyWindow *window, const char *title, int width, int height, SDL_WindowFlags window_flags)
{
    SDL_Log("\tMyWindow_initialize(%s, %d, %d)", title, width, height);

    if (!window)
    {
        SDL_Log("\t\t*** Erro: Janela/renderizador inválidos (window == NULL).");
        return false;
    }

    return SDL_CreateWindowAndRenderer(title, width, height, window_flags, &window->window, &window->renderer);
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void MyWindow_destroy(MyWindow *window)
{
    SDL_Log(">>> MyWindow_destroy()");

    if (!window)
    {
        SDL_Log("\t*** Erro: Janela/renderizador inválidos (window == NULL).");
        SDL_Log("<<< MyWindow_destroy()");
        return;
    }

    SDL_Log("\tDestruindo MyWindow->renderer...");
    SDL_DestroyRenderer(window->renderer);
    window->renderer = NULL;

    SDL_Log("\tDestruindo MyWindow->window...");
    SDL_DestroyWindow(window->window);
    window->window = NULL;

    SDL_Log("<<< MyWindow_destroy()");
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void MyImage_destroy(MyImage *image)
{
    SDL_Log(">>> MyImage_destroy()");

    if (!image)
    {
        SDL_Log("\t*** Erro: Imagem inválida (image == NULL).");
        SDL_Log("<<< MyImage_destroy()");
        return;
    }

    if (image->texture)
    {
        SDL_Log("\tDestruindo MyImage->texture...");
        SDL_DestroyTexture(image->texture);
        image->texture = NULL;
    }

    if (image->surface)
    {
        SDL_Log("\tDestruindo MyImage->surface...");
        SDL_DestroySurface(image->surface);
        image->surface = NULL;
    }

    SDL_Log("\tRedefinindo MyImage->rect...");
    image->rect.x = image->rect.y = image->rect.w = image->rect.h = 0.0f;

    SDL_Log("<<< MyImage_destroy()");
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void load_rgba32(const char *filename, SDL_Renderer *renderer, MyImage *output_image)
{
    SDL_Log(">>> load_rgba32(\"%s\")", filename);

    if (!filename)
    {
        SDL_Log("\t*** Erro: Nome do arquivo inválido (filename == NULL).");
        SDL_Log("<<< load_rgba32(\"%s\")", filename);
        return;
    }

    if (!renderer)
    {
        SDL_Log("\t*** Erro: Renderer inválido (renderer == NULL).");
        SDL_Log("<<< load_rgba32(\"%s\")", filename);
        return;
    }

    if (!output_image)
    {
        SDL_Log("\t*** Erro: Imagem de saída inválida (output_image == NULL).");
        SDL_Log("<<< load_rgba32(\"%s\")", filename);
        return;
    }

    MyImage_destroy(output_image);

    SDL_Log("\tCarregando imagem \"%s\" em uma superfície...", filename);
    SDL_Surface *surface = IMG_Load(filename);
    if (!surface)
    {
        SDL_Log("\t*** Erro ao carregar a imagem: %s", SDL_GetError());
        SDL_Log("<<< load_rgba32(\"%s\")", filename);
        return;
    }

    SDL_Log("\tConvertendo superfície para formato RGBA32...");
    output_image->surface = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_RGBA32);
    SDL_DestroySurface(surface);
    if (!output_image->surface)
    {
        SDL_Log("\t*** Erro ao converter superfície para formato RGBA32: %s", SDL_GetError());
        SDL_Log("<<< load_rgba32(\"%s\")", filename);
        return;
    }

    SDL_Log("\tCriando textura a partir da superfície...");
    output_image->texture = SDL_CreateTextureFromSurface(renderer, output_image->surface);
    if (!output_image->texture)
    {
        SDL_Log("\t*** Erro ao criar textura: %s", SDL_GetError());
        SDL_Log("<<< load_rgba32(\"%s\")", filename);
        return;
    }

    SDL_Log("\tObtendo dimensões da textura...");
    SDL_GetTextureSize(output_image->texture, &output_image->rect.w, &output_image->rect.h);

    SDL_Log("<<< load_rgba32(\"%s\")", filename);
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void convert_gray_scale_image(SDL_Renderer *renderer, MyImage *image)
{
    SDL_Log(">>> convert_gray_scale_image()");

    if (!renderer)
    {
        SDL_Log("\t*** Erro: Renderer inválido (renderer == NULL).");
        SDL_Log("<<< convert_gray_scale_image()");
        return;
    }

    if (!image || !image->surface)
    {
        SDL_Log("\t*** Erro: Imagem inválida (image == NULL ou image->surface == NULL).");
        SDL_Log("<<< convert_gray_scale_image()");
        return;
    }

    // Para acessar os pixels de uma superfície, precisamos chamar essa função.
    SDL_LockSurface(image->surface);

    const SDL_PixelFormatDetails *format = SDL_GetPixelFormatDetails(image->surface->format);
    const size_t pixelCount = image->surface->w * image->surface->h;

    Uint32 *pixels = (Uint32 *)image->surface->pixels;
    Uint8 r = 0;
    Uint8 g = 0;
    Uint8 b = 0;
    Uint8 a = 0;
    Uint8 c = 0;

    for (size_t i = 0; i < pixelCount; ++i)
    {
        SDL_GetRGBA(pixels[i], format, NULL, &r, &g, &b, &a);

        c = 0.2125 * r + 0.7154 * g + 0.0721 * b;

        pixels[i] = SDL_MapRGBA(format, NULL, c, c, c, a);
    }

    // Após manipularmos os pixels da superfície, liberamos a superfície.
    SDL_UnlockSurface(image->surface);

    // Atualizamos a textura a ser renderizada pelo SDL_Renderer, com base no
    // novo conteúdo da superfície.
    SDL_DestroyTexture(image->texture);
    image->texture = SDL_CreateTextureFromSurface(renderer, image->surface);

    SDL_Log("<<< convert_gray_scale_image()");
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void create_hist(MyImage *image) {
    SDL_Log(">>> create_hist()");

    SDL_Log("\tCriando o histograma...");

    // Limpa o vetor do histograma
    for (int i = 0; i < PIXEL_DEPTH; i++) {
        hist[i] = 0;
    }

    SDL_LockSurface(image->surface);

    const SDL_PixelFormatDetails *format = SDL_GetPixelFormatDetails(image->surface->format);
    const size_t pixelCount = image->surface->w * image->surface->h;

    Uint32 *pixels = (Uint32 *)image->surface->pixels;

    // Só um canal é necessário pois a imagem estará em escala de cinza
    Uint8 r = 0; 

    // Preenche o vetor do histograma
    for (size_t i = 0; i < pixelCount; i++) {
      SDL_GetRGB(pixels[i], format, NULL, &r, NULL, NULL);
      hist[r] += 1;
    }

    SDL_UnlockSurface(image->surface);

    // Cálculo da média e atualiza máximo
    double sum = 0;
    for (int i = 0; i < PIXEL_DEPTH; i++) {
        sum += (double) i * hist[i];

        if (hist[i] > max_hist) {
          max_hist = hist[i];
        }
    }
    mean_hist = sum / (double) pixelCount;

    // Cálculo do desvio padrão
    sum = 0;
    for (int i = 0; i < PIXEL_DEPTH; i++) {
        double diff = (double) i - mean_hist;
        sum += (diff * diff) * hist[i];
    }
    double variance = sum / (double) pixelCount;
    std_dev_hist = sqrt(variance);

    SDL_Log("<<< create_hist()");
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void draw_hist() {
    int y_axis_x = 40;
    int y_axis_min_y = 350;
    int y_axis_max_y = 40;

    int x_axis_y = 330;
    int x_axis_min_x = 20;
    int x_axis_max_x = 550;

    SDL_SetRenderDrawColor(g_childWindow.renderer, 255, 255, 255, 255);
    SDL_RenderClear(g_childWindow.renderer);

    SDL_SetRenderDrawColor(g_childWindow.renderer, 0, 0, 0, 255);
    SDL_RenderLine(g_childWindow.renderer, y_axis_x, y_axis_max_y, y_axis_x, y_axis_min_y);

    SDL_RenderLine(g_childWindow.renderer, x_axis_min_x, x_axis_y, x_axis_max_x, x_axis_y);

    int current_x = y_axis_x + 1;
    int current_y = x_axis_y;
    for (int i = 0; i < PIXEL_DEPTH; i++) {
      if (hist[i] == 0) {
        continue;
      }
      // Normalização dos valores de frequência
      current_y = x_axis_y - ((hist[i] * (x_axis_y - y_axis_max_y)) / max_hist);
      SDL_RenderLine(g_childWindow.renderer, current_x, x_axis_y, current_x, current_y);
      current_x += 2;
    }

    SDL_RenderPresent(g_childWindow.renderer);
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
static SDL_AppResult initialize(void)
{
    SDL_Log(">>> initialize()");

    SDL_Log("\tIniciando SDL...");
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        SDL_Log("\t*** Erro ao iniciar a SDL: %s", SDL_GetError());
        SDL_Log("<<< initialize()");
        return SDL_APP_FAILURE;
    }

    if (!TTF_Init())//inicializacao do ttf
    {
        SDL_Log("\t*** Erro ao iniciar SDL_ttf: %s", SDL_GetError());
        SDL_Log("<<< initialize()");
        return SDL_APP_FAILURE;
    }

    g_font = TTF_OpenFont(FONT_FILENAME, 16.0f);
    if (!g_font)
    {
        SDL_Log("\t*** Erro ao abrir fonte: %s", SDL_GetError());
        SDL_Log("<<< initialize()");
        return SDL_APP_FAILURE;
    }

    SDL_Log("\tCriando janela principal e renderizador...");
    if (!MyWindow_initialize(&g_window, WINDOW_TITLE, DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT, 0))
    {
        SDL_Log("\t*** Erro ao criar a janela principal e/ou renderizador: %s", SDL_GetError());
        SDL_Log("<<< initialize()");
        return SDL_APP_FAILURE;
    }

    SDL_Log("\tCriando janela secundária e renderizador...");
    if (!MyWindow_initialize(&g_childWindow, _TITLE, DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT, 0))
    {
        SDL_Log("\t*** Erro ao criar a janela secundária e/ou renderizador: %s", SDL_GetError());
        SDL_Log("<<< initialize()");
        return SDL_APP_FAILURE;
    }

    SDL_Log("\tDefinindo janela secundária como filha da principal...");
    if (!SDL_SetWindowParent(g_childWindow.window, g_window.window))
    {
        SDL_Log("\t*** Erro ao definir a janela secundária como filha: %s", SDL_GetError());
        SDL_Log("<<< initialize()");
        return SDL_APP_FAILURE;
    }

    SDL_Log("<<< initialize()");
    return SDL_APP_CONTINUE;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
static void shutdown(void)
{
    SDL_Log(">>> shutdown()");

    if (g_font) //complementa para encerrar o ttf
    {
        TTF_CloseFont(g_font);
        g_font = NULL;
    }

    MyImage_destroy(&g_image);
    MyWindow_destroy(&g_childWindow);
    MyWindow_destroy(&g_window);

    TTF_Quit();

    SDL_Log("\tEncerrando SDL...");
    SDL_Quit();

    SDL_Log("<<< shutdown()");
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
static void render(void)
{
    SDL_SetRenderDrawColor(g_window.renderer, 128, 128, 128, 255);
    SDL_RenderClear(g_window.renderer);
    SDL_SetRenderDrawColor(g_childWindow.renderer, 255, 255, 255, 255);
    SDL_RenderClear(g_childWindow.renderer);

    SDL_RenderTexture(g_window.renderer, g_image.texture, &g_image.rect, &g_image.rect);

    SDL_RenderPresent(g_window.renderer);

    draw_hist();

    SDL_RenderPresent(g_childWindow.renderer);
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
static void loop(void)
{
    SDL_Log(">>> loop()");

    // Para melhorar o uso da CPU (e consumo de energia), só atualizaremos o
    // conteúdo da janela se realmente for necessário.
    bool mustRefresh = false;
    render();

    SDL_Event event;
    bool isRunning = true;
    while (isRunning)
    {
        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
            case SDL_EVENT_QUIT:
                isRunning = false;
                break;
            }
        }

        if (mustRefresh)
        {
            render();
            mustRefresh = false;
        }
    }
    
    SDL_Log("<<< loop()");
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
int main(int argc, char *argv[])
{
    atexit(shutdown);

    if (initialize() == SDL_APP_FAILURE)
        return SDL_APP_FAILURE;

    IMAGE_FILENAME = argv[1];

    load_rgba32(IMAGE_FILENAME, g_window.renderer, &g_image);

    // Verifica se a imagem já vem em escala de cinza
    SDL_Surface *image = IMG_Load(IMAGE_FILENAME);
    SDL_LockSurface(image);
    Uint32* pixels = (Uint32*)image->pixels;
    bool escalaCinza = true;
    Uint8 r, g, b;

    for (int i = 0; i < image->w * image->h; ++i) {
        SDL_GetRGB(pixels[i], SDL_GetPixelFormatDetails(image->format), NULL, &r, &g, &b);
        if (r != g || g != b) {
            escalaCinza = false;
            break;
        }
    }

    // Altera tamanho da janela e reposiciona no centro da tela.
    int imageW = (int)g_image.rect.w;
    int imageH = (int)g_image.rect.h;

    SDL_SetWindowSize(g_window.window, imageW, imageH);
    SDL_SetWindowPosition(g_window.window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);

    // Obtém o tamanho da borda das janelas e do monitor. 
    int childH = 0;
    int childW = 0;
    SDL_GetWindowSize(g_childWindow.window, &childW, &childH);

    const int padding = 16;

    // Pega a posição da janela pai
    int parentX = 0;
    int parentY = 0;
    SDL_GetWindowPosition(g_window.window, &parentX, &parentY);

    // Coloca a posição da janela filho à direita da janela pai,
    // com um espaço de "padding" pixels entre elas
    SDL_SetWindowPosition(g_childWindow.window,
        parentX + imageW + padding, parentY + (imageH - childH) / 2);

    // Caso não esteja em escala de cinza, converte a imagem
    if (!escalaCinza) convert_gray_scale_image(g_window.renderer, &g_image);

    SDL_SyncWindow(g_window.window);
    SDL_SyncWindow(g_childWindow.window);

    create_hist(&g_image);

    draw_hist();

    loop();

    return 0;
}
