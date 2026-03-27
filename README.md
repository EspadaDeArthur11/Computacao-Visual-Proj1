# Projeto 1 - Processamento de Imagens com SDL3

## Descrição

Este projeto foi desenvolvido para carregar uma imagem informada pela linha de comando e exibi-la em uma janela principal, enquanto uma segunda janela mostra o 
histograma correspondente. Durante a execução, o programa verifica se a imagem já está em escala de cinza e, caso não esteja, realiza a conversão. A partir da 
imagem processada, ele calcula valores como média e desvio padrão do histograma e apresenta essas informações em forma de texto na janela secundária com o uso 
da biblioteca SDL_ttf. Além disso, o usuário pode salvar a imagem atualmente exibida pressionando a tecla S, gerando oarquivo output_image.png.

## Bibliotecas usadas

Para compilar e executar o projeto, é necessário ter estas bibliotecas instaladas no sistema:

- SDL3
- SDL3_image
- SDL3_ttf

No projeto:
- SDL3 é usada para janela, renderização e eventos;
- SDL3_image é usada para carregar imagens;
- SDL3_ttf é usada para desenhar os textos do histograma.

## Como preparar a fonte TTF

Este projeto usa uma fonte .ttf para escrever o texto na janela do histograma.

No código atual, o caminho configurado para a fonte é:

```c
static const char *FONT_FILENAME = "font/ARIAL.ttf";
