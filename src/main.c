#include <stdio.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "driver/i2c_master.h"
#include "esp_log.h"

static const char *TAG = "FOGUETE_amand";

// Definições de Pinos
#define BUZZER_PIN       12
#define SERVO_PIN        13
#define I2C_MASTER_SDA   21
#define I2C_MASTER_SCL   22

// Configurações do Servo (PWM via LEDC)
#define LEDC_TIMER       LEDC_TIMER_0
#define LEDC_MODE        LEDC_LOW_SPEED_MODE
#define LEDC_CHANNEL     LEDC_CHANNEL_0
#define LEDC_DUTY_RES    LEDC_TIMER_13_BIT 
#define LEDC_FREQUENCY   50                

#define SERVO_MIN_DUTY   205 // 0° (Fechado)
#define SERVO_MAX_DUTY   614 // 90° (Aberto)

#define BMP085_I2C_ADDR  0x77
#define TAMANHO_FILTRO   8   // Média móvel de 8 amostras

// Sensibilidade da detecção do apogeu
#define LIMIAR_SUBIDA        4.0f    // Diferença para detectar lançamento
#define LIMIAR_DESCIDA       4.0f    // Diferença mínima para confirmar descida
#define CONFIRMACAO_DESCIDA  5       // Número de leituras consecutivas

// Handlers do FreeRTOS
QueueHandle_t xAltitudeQueue;
TaskHandle_t xActuatorTaskHandle = NULL;
i2c_master_dev_handle_t dev_handle;

// --- FUNÇÕES DE HARDWARE ---
void init_hardware(void) {
    gpio_reset_pin(BUZZER_PIN);
    gpio_set_direction(BUZZER_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(BUZZER_PIN, 0);

    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_MODE,
        .duty_resolution  = LEDC_DUTY_RES,
        .timer_num        = LEDC_TIMER,
        .freq_hz          = LEDC_FREQUENCY,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ledc_timer_config(&ledc_timer);

    ledc_channel_config_t ledc_channel = {
        .speed_mode     = LEDC_MODE,
        .channel        = LEDC_CHANNEL,
        .timer_sel      = LEDC_TIMER,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = SERVO_PIN,
        .duty           = SERVO_MIN_DUTY, 
        .hpoint         = 0
    };
    ledc_channel_config(&ledc_channel);
}

void init_i2c(void) {
    i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = I2C_MASTER_SDA,
        .scl_io_num = I2C_MASTER_SCL,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    i2c_master_bus_handle_t bus_handle;
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &bus_handle));

    i2c_device_config_t dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = BMP085_I2C_ADDR,
        .scl_speed_hz = 100000,
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_config, &dev_handle));
}

float ler_pressao_bruta(void) {
    uint8_t cmd_reg[2] = {0xF4, 0x34}; 
    i2c_master_transmit(dev_handle, cmd_reg, 2, -1);
    vTaskDelay(pdMS_TO_TICKS(10)); 

    uint8_t data_reg = 0xF6;
    uint8_t leitura[2] = {0};
    i2c_master_transmit_receive(dev_handle, &data_reg, 1, leitura, 2, -1);

    return (float)((leitura[0] << 8) | leitura[1]);
}

// --- TASKS FREERTOS ---

// 1. TASK DO SENSOR
void vTaskSensor(void *pvParameters)
{
    float historico[TAMANHO_FILTRO] = {0};
    int indice = 0;

    float leitura_inicial = ler_pressao_bruta();

    for(int i = 0; i < TAMANHO_FILTRO; i++)
    {
        historico[i] = leitura_inicial;
    }

    for (;;)
    {
        historico[indice] = ler_pressao_bruta();
        indice = (indice + 1) % TAMANHO_FILTRO;

        float soma = 0;

        for (int i = 0; i < TAMANHO_FILTRO; i++)
        {
            soma += historico[i];
        }

        float pressao_filtrada = soma / (float)TAMANHO_FILTRO;

        // <<< COLOQUE AQUI >>>
        ESP_LOGI(TAG, "Pressao = %.2f", pressao_filtrada);

        xQueueSend(xAltitudeQueue, &pressao_filtrada, portMAX_DELAY);

        vTaskDelay(pdMS_TO_TICKS(80));
    }
}

// 2. TASK DE PROCESSAMENTO
void vTaskProcess(void *pvParameters)
{
    float pressaoAtual;

    float pressaoMesa;
    float menorPressao;

    int contagemQueda = 0;

    /*
        Estados:

        0 -> Armado
        1 -> Subindo
        2 -> Apogeu confirmado
    */

    int estadoVoo = 0;

    // Aguarda estabilizar
    vTaskDelay(pdMS_TO_TICKS(2000));

    xQueueReceive(xAltitudeQueue, &pressaoAtual, portMAX_DELAY);

    pressaoMesa = pressaoAtual;
    menorPressao = pressaoAtual;

    ESP_LOGI(TAG, "Pressao inicial: %.2f", pressaoMesa);

    while (1)
    {
        if (xQueueReceive(xAltitudeQueue, &pressaoAtual, portMAX_DELAY) == pdPASS)
        {
            ESP_LOGI(TAG,
                     "P=%.2f | Menor=%.2f | Estado=%d | Confirmacao=%d/%d",
                     pressaoAtual,
                     menorPressao,
                     estadoVoo,
                     contagemQueda,
                     CONFIRMACAO_DESCIDA);

            switch (estadoVoo)
            {

            // ==========================================================
            // ESTADO ARMADO
            // ==========================================================

            case 0:

                // Detecta subida
                if (pressaoAtual < (pressaoMesa - LIMIAR_SUBIDA))
                {
                    estadoVoo = 1;

                    menorPressao = pressaoAtual;

                    ESP_LOGW(TAG, "====================================");
                    ESP_LOGW(TAG, "SUBIDA DETECTADA");
                    ESP_LOGW(TAG, "====================================");
                }

                break;

            // ==========================================================
            // ESTADO SUBINDO
            // ==========================================================

            case 1:

                /*
                    Se encontrou uma pressão ainda menor,
                    significa que ainda está subindo.
                */
                if (pressaoAtual < menorPressao)
                {
                    menorPressao = pressaoAtual;

                    contagemQueda = 0;
                }

                /*
                    Só considera descida quando a pressão
                    aumentar mais que o limiar.
                */
                else if (pressaoAtual > (menorPressao + LIMIAR_DESCIDA))
                {
                    contagemQueda++;

                    ESP_LOGW(TAG,
                             "Descida confirmando (%d/%d)",
                             contagemQueda,
                             CONFIRMACAO_DESCIDA);

                    if (contagemQueda >= CONFIRMACAO_DESCIDA)
                    {
                        estadoVoo = 2;

                        ESP_LOGE(TAG, "");
                        ESP_LOGE(TAG, "########################################");
                        ESP_LOGE(TAG, "######## APOGEU DETECTADO ########");
                        ESP_LOGE(TAG, "########################################");

                        xTaskNotifyGive(xActuatorTaskHandle);
                    }
                }

                else
                {
                    /*
                        Está apenas oscilando próximo ao apogeu.
                        Não conta como descida.
                    */

                    contagemQueda = 0;
                }

                break;

            // ==========================================================
            // ESTADO EJETADO
            // ==========================================================

            case 2:

                // Apenas permanece parado

                break;
            }
        }
    }
}

// 3. TASK DOS ATUADORES
void vTaskActuators(void *pvParameters) {
    for (;;) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, SERVO_MAX_DUTY);
        ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);
        ESP_LOGI(TAG, "-> Servo ativado em 90 graus.");

        for (int i = 0; i < 10; i++) {
            gpio_set_level(BUZZER_PIN, 1);
            vTaskDelay(pdMS_TO_TICKS(100));
            gpio_set_level(BUZZER_PIN, 0);
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
}

void app_main(void) {
    ESP_LOGI(TAG, "Iniciando Versao de Bancada com Filtro Suave...");
    init_hardware();
    init_i2c();

    xAltitudeQueue = xQueueCreate(5, sizeof(float));

    if (xAltitudeQueue != NULL) {
        xTaskCreate(vTaskSensor,    "Task_Sensor",    3072, NULL, 2, NULL);
        xTaskCreate(vTaskProcess,   "Task_Process",   3072, NULL, 3, NULL);
        xTaskCreate(vTaskActuators, "Task_Actuators", 3072, NULL, 4, &xActuatorTaskHandle);
    }
}
