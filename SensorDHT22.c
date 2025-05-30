#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include <string.h>
#include "lcd.h"


const int GPIO_DHT = 26;
const int TIMEOUT = 1000;

//Função para medir o pulso do sensor DHT22
//bool level refere-se ao nível do pulso (0) = baixo; (1) = alto
static uint32_t measure_pulse(bool level){

  //Captura o tempo atual de microssegundos em 32bits
  uint32_t start_time = time_us_32();

  //Laço de repetição para conferir o tempo de pulso do nível sensor DHT22
  while(gpio_get(GPIO_DHT) == level){
    if (time_us_32() - start_time > TIMEOUT){
      return TIMEOUT;
    }
  }
  //Retorno caso o tempo de pulso for menor que TIMEOUT
  return time_us_32() - start_time;

}

//Função que irá sempre inicar um pulso para o DHT22, informado que será iniciado a comunicação
void send_initial_pulse(){

  //O microcontrolador informa ao sensor o início da comunicação
  gpio_set_dir(GPIO_DHT, GPIO_OUT);
  gpio_put(GPIO_DHT, 0);// Pulso de nível baixo, dura até 18 milissegundos
  sleep_ms(18);

  //O microcontrolador espera uma resposta do sensor
  gpio_put(GPIO_DHT, 1);// Pulso de nível alto para receber a resposta
  sleep_us(30);// sleep em microssegundos
  
  //Colocar o sensor em modo de Entrada
  gpio_set_dir(GPIO_DHT, GPIO_IN);
}

//Função para soma da estrutura data
uint8_t check_sum(uint8_t *data){

  const uint8_t DATA_LENGTH = 4; 
  uint8_t sum = 0;

  for(int i = 0; i < DATA_LENGTH; i++){
    sum=+ data[i];
  }

  return sum;
}


//Função que pegará os dados do sensor DHT22
bool read_data_dht22(uint8_t *data){

  //Zera o buffer da estrutura data
  memset(data,0,5);

  //Confere os tempos de pulso do sensor em nível Alto (1) e nível Baixo(0)
  if(measure_pulse(0) == TIMEOUT || measure_pulse(1) == TIMEOUT){
    
    printf("Erro na leitura do sensor: TIMEOUT-READ-DATA \n");
    return false;

  }

  //Faz a leitura dos 40 bits do sensor
  //2 bytes para a temperatura
  //2 bytes para a umidade
  //1 byte com a soma/checksum de temperatura e umidade
  for(int i = 0; i < 40; i++ ){
    
    if(measure_pulse(0) == TIMEOUT){

      printf("Erro na leitura do sensor: TIMEOUT-LOW-PULSE \n");
      return false;
    }

    //Condicional para o tempo atual
    uint32_t time_duration = measure_pulse(1);
    if(time_duration == TIMEOUT){
      
      printf("Erro na leitura do sensor: TIMEOUT-HIGH-PULSE \n");
      return false;

    }

    //Confere se o time_duration está certo para início do envio das informações
    //
    if(time_duration > 50){
      data[i / 8] |= (1 << (7 - (i % 8)));
      printf("DADOS SALVOS!!! \n");
    }

  }

  //Condicional para o checksum
  uint8_t checksum = data[0] + data[1] + data[2] + data[3];

  if(checksum != data[4]){
    printf("Erro na soma dos dados: CHECK-SUM-ERROR \n");
    return false;
  }

  return true;

}

//Função para concatenar a variável temperatura com o buffer
void concatenate_temperature(float temperature, char* buffer, size_t size) {
    snprintf(buffer, size, "Temp: %.2f", temperature);
}

//Função para concatenar a variável umidity com o buffer
void concatenate_umidity(float umidity, char* buffer, size_t size) {
    snprintf(buffer, size, "Umidity: %.2f %%", umidity);
}

//Função para mostrar informações na tela
void display_info(char *temperature, char *umidity){

  char *message[] = {
    temperature,
    umidity,
  };

  for (uint m = 0; m < sizeof(message) / sizeof(message[0]); m += MAX_LINES) {
    for (int line = 0; line < MAX_LINES; line++) {
      lcd_set_cursor(line, (MAX_CHARS / 2) - strlen(message[m + line]) / 2);
      lcd_string(message[m + line]);
    }
  }
}

//Função principal
int main() {
  stdio_init_all();
  gpio_init(GPIO_DHT);
  sda_scl_init();
  lcd_init();

  uint8_t data [5];
  float temperature, umidity;
  char buffer_temp[32];
  char buffer_umi[32];

  
  while (true) {
    printf("INICIANDO LEITURA DO SENSOR \n");

    send_initial_pulse();

    //Conversão de valores para números reais
    if(read_data_dht22(data)){
      umidity = ((data [0] << 8) + data[1]) / 10.0;
      temperature = ((data[2] << 8) + data[3]) / 10.0;

      if(temperature < 0.0){
        temperature = 0.0;
      }

    }

    printf("Temperatura: %.2f º | Umidade: %.2f %% \n", temperature, umidity);

    concatenate_temperature(temperature, buffer_temp, sizeof(buffer_temp));
    concatenate_umidity(umidity, buffer_umi, sizeof(buffer_umi));

    display_info(buffer_temp, buffer_umi);

    //Sleep para acontecer leitura apenas a cada 2 segundos.
    sleep_ms(2000);
  }
}