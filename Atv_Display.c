// Bibliotecas Padrão
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "inc/ssd1306.h"
#include "inc/font.h"
#include "hardware/clocks.h"
#include "hardware/gpio.h"
#include "ws2812.pio.h"
#include "hardware/pio.h"
#include "Matriz.h"

// Definição para o display OLED
#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define endereco 0x3C

// Definição para os LEDS RGB
#define LED_GREEN 11
#define LED_BLUE 12

// Definição para os botões
#define BOTAO_A 5
#define BOTAO_B 6

// Definição para a matriz de LEDS
#define NUM_LEDS 25
#define PIN 7

// Definição para a UART
#define UART_ID uart0
#define BAUD_RATE 115200
#define UART_TX_PIN 0
#define UART_RX_PIN 1


// Função para animar a matriz de LEDS
void atualizar_matriz(int numero, PIO pio, uint sm) {
    if (numero >= 1 && numero <= 9) {
        double* desenho = desenhos[numero - 1]; // Pega o desenho correspondente ao número
        animacao(desenho, NUM_LEDS, pio, sm, 0.3, 0.0, 0.0); // Anima o desenho
    }
}

// Prototipo da função de interrupção
static void botao_pressionado(uint gpio, uint32_t events); 

// Armazena o tempo do último evento (em microssegundos)
static volatile uint32_t ultimo_evento = 0; 

// Estados dos LEDs
bool estado_ledG = false;
bool estado_ledB = false;

// Inicializa a estrutura do display
ssd1306_t ssd; 


// Função principal
int main(){
    stdio_init_all(); // Inicializa a comunicação serial
    
    // Inicializa a UART com o baud rate especificado
    uart_init(UART_ID, BAUD_RATE);

    // Configura os pinos GPIO para a função UART
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);

    // Configura o formato da UART (8 bits de dados, 1 bit de parada, sem paridade)
    uart_set_format(UART_ID, 8, 1, UART_PARITY_NONE);

    // Habilita a FIFO da UART para evitar perda de dados
    uart_set_fifo_enabled(UART_ID, true);

    PIO pio = pio0; // Inicialização do pio
    sleep_ms(500); // Delay para evitar sobrecarga

    printf("Iniciando...\n");
    
    bool verificado = set_sys_clock_khz(128000, false); // Configuração do clock do sistema


    ///////////////////////// INICIALIZANDO I2C /////////////////////////
    // Inicialização do display OLED com 400000 Hz
    i2c_init(I2C_PORT, 400 * 1000);

    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C); // Define a função do pino GPIO como I2C
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C); // Define a função do pino GPIO como I2C
    gpio_pull_up(I2C_SDA); // Pull up no pino de dados
    gpio_pull_up(I2C_SCL); // Pull up no pino de clock
   
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT); // Inicializa o display
    ssd1306_config(&ssd); // Configura o display
    ssd1306_send_data(&ssd); // Envia os dados para o display

    // Limpa o display. O display inicia com todos os pixels apagados.
    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);
    
    // Inicializa LEDS RGB
    gpio_init(LED_GREEN);
    gpio_set_dir(LED_GREEN, GPIO_OUT);
    gpio_init(LED_BLUE);
    gpio_set_dir(LED_BLUE, GPIO_OUT);

    // Inicializa botao A 
    gpio_init(BOTAO_A);
    gpio_set_dir(BOTAO_A, GPIO_IN);
    gpio_pull_up(BOTAO_A);
    // Inicializa botao B
    gpio_init(BOTAO_B);
    gpio_set_dir(BOTAO_B, GPIO_IN);
    gpio_pull_up(BOTAO_B);

    // Interrupção dos botões
    gpio_set_irq_enabled_with_callback(BOTAO_A, GPIO_IRQ_EDGE_FALL, true, &botao_pressionado);
    gpio_set_irq_enabled_with_callback(BOTAO_B, GPIO_IRQ_EDGE_FALL, true, &botao_pressionado);

    // Configuração PIO

    // Adiciona o programa ao pio às maquinas de estado
    uint offset = pio_add_program(pio, &Atv_Display_program);
    // Reivindica uma máquina de estado livre para executar o programa PIO
    uint sm = pio_claim_unused_sm(pio, true); 
    // Configura a SM para executar o programa PIO
    Atv_Display_program_init(pio, sm, offset, PIN, 800000, false); 


    // Loop principal
    while (true)
{   // Verifica se a UART está conectada
    if(stdio_usb_connected){
        char c; // Variável para armazenar o caractere recebido
        printf("Digite um caractere ou um número de 0 a 9: ");
        if(scanf("%c", &c) == 1){
            printf("Texto recebido %c\n", c);

            ssd1306_fill(&ssd, false); // Limpa o display

            // Verifica se a tecla digitada é D, d, L ou l
            if (c == 'D' || c == 'd' || c == 'L' || c == 'l'){
                // Exibe o caractere D, d, L ou l
                char texto[2] = {c, '\0'}; // Cria uma string com o caractere
                ssd1306_draw_string(&ssd, "CARACTERE       RECEBIDO", 20, 30); // Desenha uma string
                ssd1306_draw_string(&ssd, texto, 45, 50); // Desenha o caractere no display
                ssd1306_send_data(&ssd); // Atualiza o display

                // Realiza a ação correspondente ao caractere digitado
                if (c == 'D' || c == 'd'){
                    printf("Desligando Matriz de LEDS...\n");
                    animacao(NULL, 0, pio0, sm, 0.0, 0.0, 0.0); // Desliga a Matriz
                } else if (c == 'L' || c == 'l'){
                    printf("Ligando Matriz de LEDS...\n");
                    animacao(desenhos[0], 0, pio0, sm, 0.3, 0.0, 0.0); // Liga a Matriz
                }
            }
            // Se for um número de 0 a 9, exibe no display
            else if (c >= '0' && c <= '9') {
                int numero = c - '0'; // Converte o caractere para um número
                ssd1306_fill(&ssd, false); // Limpa o display
                char texto[2] = {c, '\0'}; // Cria uma string com o número recebido
                ssd1306_draw_string(&ssd, "MATRIZ ATUALIZADA", 10, 15); // Desenha uma string
                ssd1306_draw_string(&ssd, "NUMERO", 25, 37); // Desenha uma string
                ssd1306_draw_string(&ssd, texto, 40, 50); // Desenha uma string
                ssd1306_send_data(&ssd); // Atualiza o display

                animacao(desenhos[numero], 0, pio0, sm, 0.3, 0.0, 0.0); // Anima o desenho
            } else {
                ssd1306_fill(&ssd, false); // Limpa o display
                char texto[2] = {c, '\0'}; // Cria uma string com o caractere recebido
                ssd1306_draw_string(&ssd, "CARACTERE       RECEBIDO", 20, 30); // Desenha uma string
                ssd1306_draw_string(&ssd, texto, 45, 50); // Desenha a string no display
                ssd1306_send_data(&ssd); // Atualiza o display
            }
        }
        sleep_ms(1000); // Delay para evitar sobrecarga
    }    
}
    
}

// Função de interrupção para os botões
void botao_pressionado(uint gpio, uint32_t events)
{
   
    // Verifica se o tempo entre os eventos é maior que 300ms
    uint32_t current_time = to_us_since_boot(get_absolute_time());

    // Se o tempo entre os eventos for maior que 300ms, o evento é registrado
    if (current_time - ultimo_evento > 300000) 
    {
        // Atualiza o tempo do último evento para o tempo atual
        ultimo_evento = current_time;

        
        
        // Verifica qual botão foi pressionado
       if (gpio == BOTAO_A)
        
        {   estado_ledG = !estado_ledG; // Inverte o estado do LED
            gpio_put(LED_GREEN, estado_ledG); // Atualiza o estado do LED

            printf("Botão A pressionado\n");
            ssd1306_fill(&ssd, false); // Limpa o display
            ssd1306_draw_string(&ssd, estado_ledG ? "LED VERDE: ON" : "LED VERDE: OFF", 10, 30); // Desenha uma string
            ssd1306_send_data(&ssd); // Atualiza o display

           
        }
        if (gpio == BOTAO_B)
        {
            estado_ledB = !estado_ledB; // Inverte o estado do LED
            gpio_put(LED_BLUE, estado_ledB); // Atualiza o estado do LED

            printf("Botão B pressionado\n");
            ssd1306_fill(&ssd, false); // Limpa o display
            ssd1306_draw_string(&ssd, estado_ledB ?  "LED AZUL: ON" : "LED AZUL: OFF", 10, 30); // Desenha uma string
            ssd1306_send_data(&ssd); // Atualiza o display
            
            
        }
   }
}


