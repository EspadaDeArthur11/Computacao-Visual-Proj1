// Copyright (c) 2026 Andre Kishimoto - https://kishimoto.com.br/
// SPDX-License-Identifier: Apache-2.0

//------------------------------------------------------------------------------
// Exemplo: 04-invert_image
// O programa carrega o arquivo de imagem indicado na constante IMAGE_FILENAME
// e exibe o conteúdo na janela ("kodim23.png" pertence ao "Kodak Image Set").
// A tecla '1' aplica uma transformação de intensidade (negativo da imagem).
// Caso a imagem seja maior do que WINDOW_WIDTHxWINDOW_HEIGHT, a janela é
// redimensionada logo após a imagem ser carregada.
//
// Observação:
// Em um projeto mais realista, o código abaixo provavelmente seria refatorado.
// Alguns exemplos de refatoração do projeto:
// - Uso de headers (.h) e outros arquivos .c (ex. estruturas e operações
//   relacionadas à imagens);
// - Remoção de variáveis globais;
// - Redução de logs (ou melhor, seriam desativados na build release);
// - Arquivo de imagem seria um parâmetro do programa (argv), ao invés de ser
//   uma string constante IMAGE_FILENAME.
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_image/SDL_image.h>

//------------------------------------------------------------------------------
// Custom types, structs, constants, etc.
//------------------------------------------------------------------------------
static const char *WINDOW_TITLE = "Imagem";
static const char *CHILD_WINDOW_TITLE = "Histograma";
static const char *IMAGE_FILENAME = "kodim23.png";

enum constants
{
    DEFAULT_WINDOW_WIDTH = 640,
    DEFAULT_WINDOW_HEIGHT = 480,
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
// Globals (argh!)
//------------------------------------------------------------------------------
static MyWindow g_window = { .window = NULL, .renderer = NULL };
static MyWindow g_childWindow = { .window = NULL, .renderer = NULL };
static MyImage g_image = {
    .surface = NULL,
    .texture = NULL,
    .rect = { .x = 0.0f, .y = 0.0f, .w = 0.0f, .h = 0.0f }
};

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

/**
 * Acessa cada pixel da imagem (MyImage->surface) e inverte sua intensidade.
 * Altera MyImage->surface e atualiza MyImage->texture.
 * 
 * Assumimos que os pixels da imagem estão no formato RGBA32 e que os níveis de
 * intensidade estão no intervalo [0-255].
 * 
 * Na verdade, podemos desconsiderar o canal Alpha, já que ele não terá seu
 * valor invertido. Neste caso, substituímos `SDL_GetRGBA()` e `SDL_MapRGBA()`
 * por `SDL_GetRGB()` e `SDL_MapRGB()`, respectivamente.
 */
static void invert_image(SDL_Renderer *renderer, MyImage *image);

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

void invert_image(SDL_Renderer *renderer, MyImage *image)
{
    SDL_Log(">>> invert_image()");

    if (!renderer)
    {
        SDL_Log("\t*** Erro: Renderer inválido (renderer == NULL).");
        SDL_Log("<<< invert_image()");
        return;
    }

    if (!image || !image->surface)
    {
        SDL_Log("\t*** Erro: Imagem inválida (image == NULL ou image->surface == NULL).");
        SDL_Log("<<< invert_image()");
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

    for (size_t i = 0; i < pixelCount; ++i)
    {
        SDL_GetRGBA(pixels[i], format, NULL, &r, &g, &b, &a);

        r = 255 - r;
        g = 255 - g;
        b = 255 - b;

        pixels[i] = SDL_MapRGBA(format, NULL, r, g, b, a);
    }

    // Após manipularmos os pixels da superfície, liberamos a superfície.
    SDL_UnlockSurface(image->surface);

    // Atualizamos a textura a ser renderizada pelo SDL_Renderer, com base no
    // novo conteúdo da superfície.
    SDL_DestroyTexture(image->texture);
    image->texture = SDL_CreateTextureFromSurface(renderer, image->surface);

    SDL_Log("<<< invert_image()");
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

    SDL_Log("\tCriando janela principal e renderizador...");
    if (!MyWindow_initialize(&g_window, WINDOW_TITLE, DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT, 0))
    {
        SDL_Log("\t*** Erro ao criar a janela principal e/ou renderizador: %s", SDL_GetError());
        SDL_Log("<<< initialize()");
        return SDL_APP_FAILURE;
    }

    SDL_Log("\tCriando janela secundária e renderizador...");
    if (!MyWindow_initialize(&g_childWindow, CHILD_WINDOW_TITLE, DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT, 0))
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

    MyImage_destroy(&g_image);
    MyWindow_destroy(&g_window);

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
    SDL_SetRenderDrawColor(g_childWindow.renderer, 128, 128, 128, 255);
    SDL_RenderClear(g_childWindow.renderer);

    SDL_RenderTexture(g_window.renderer, g_image.texture, &g_image.rect, &g_image.rect);

    SDL_RenderPresent(g_window.renderer);

    SDL_RenderTexture(g_childWindow.renderer, g_image.texture, &g_image.rect, &g_image.rect);

    SDL_RenderPresent(g_childWindow.renderer);
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
static void loop(void)
{
    SDL_Log(">>> loop()");

    // Para melhorar o uso da CPU (e consumo de energia), só atualizaremos o
    // conteúdo da janela se realmente for necessário. Nesse exemplo, isso
    // acontece quando invertemos os pixels da imagem.
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

            case SDL_EVENT_KEY_DOWN:
                if (event.key.key == SDLK_1 && !event.key.repeat)
                {
                    invert_image(g_window.renderer, &g_image);
                    mustRefresh = true;
                }
                if (event.key.key == SDLK_2 && !event.key.repeat)
                {
                    mustRefresh = true;
                }
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

    // Altera tamanho da janela se a imagem for maior do que o tamanho padrão
    // e reposiciona no centro da tela.
    int imageW = (int)g_image.rect.w;
    int imageH = (int)g_image.rect.h;

    SDL_SetWindowSize(g_window.window, imageW, imageH);
    SDL_SetWindowPosition(g_window.window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);

    // Obtém o tamanho da borda das janelas e do monitor. 
    int childH = 0;
    int childW = 0;
    SDL_GetWindowSize(g_childWindow.window, &childW, &childH);

    const int meio = 16;

    // Pega a posição da janela pai
    int parentX = 0;
    int parentY = 0;
    SDL_GetWindowPosition(g_window.window, &parentX, &parentY);

    // Coloca a posição da janela filho à direita da janela pai,
    // com um espaço de "meio" pixels entre elas
    SDL_SetWindowPosition(g_childWindow.window,
        parentX + imageW + meio, parentY + (imageH - childH) / 2);

    // Caso não esteja em escala de cinza, converte a imagem
    if (!escalaCinza) convert_gray_scale_image(g_window.renderer, &g_image);

    SDL_SyncWindow(g_window.window);
    SDL_SyncWindow(g_childWindow.window);

    loop();

    return 0;
}

