#include <Wire.h>

// Pinos do Arduino Uno ligados ao PIR, LED, buzzer, ultrassônico e botão.
const byte pir = 6, led = 7, buzzer = 8, trig = 9, echo = 10, botao = 4;
// O pulso de retorno dura cerca de 58 microssegundos por centímetro.
const unsigned long limiteUs = 30UL * 58;
const unsigned long tempoLuz = 3000;

bool protecao = true, perto = false, alarme = false;
bool luz = false;
int leituraBotao = HIGH, botaoEstavel = HIGH;
int telaAtual = -1;
unsigned long mudouBotao = 0, ultimaMedicao = 0, ultimoMovimento = 0;

// Envia comandos e caracteres ao LCD pelo módulo I2C no endereço 0x27.
void expansor(byte valor) {
  Wire.beginTransmission(0x27);
  Wire.write(valor | 0x08);
  Wire.endTransmission();
}

void nibbleLCD(byte valor) {
  expansor(valor);
  expansor(valor | 0x04);
  delayMicroseconds(1);
  expansor(valor);
  delayMicroseconds(50);
}

void enviarLCD(byte valor, bool caractere) {
  byte modo = caractere ? 1 : 0;
  nibbleLCD((valor & 0xF0) | modo);
  nibbleLCD(((valor << 4) & 0xF0) | modo);
}

void linhaLCD(byte endereco, const char *texto) {
  enviarLCD(endereco, false);
  for (byte coluna = 0; coluna < 16; coluna++) {
    enviarLCD(*texto ? *texto++ : ' ', true);
  }
}

// Atualiza o LCD somente quando a mensagem muda.
void mostrar(int tela, const char *linha1, const char *linha2) {
  if (tela == telaAtual) return;
  telaAtual = tela;
  linhaLCD(0x80, linha1);
  linhaLCD(0xC0, linha2);
}

void setup() {
  pinMode(pir, INPUT);
  pinMode(led, OUTPUT);
  pinMode(buzzer, OUTPUT);
  pinMode(trig, OUTPUT);
  pinMode(echo, INPUT);
  // Com o pull-up interno, o botão pressionado liga D4 ao GND.
  pinMode(botao, INPUT_PULLUP);

  Wire.begin();
  delay(50);
  nibbleLCD(0x30);
  delay(5);
  nibbleLCD(0x30);
  delayMicroseconds(150);
  nibbleLCD(0x30);
  nibbleLCD(0x20);
  enviarLCD(0x28, false);
  enviarLCD(0x0C, false);
  enviarLCD(0x01, false);
  delay(2);
  enviarLCD(0x06, false);
  mostrar(6, "Mona Lisa", "Obra protegida");
}

// Espera a leitura do botão estabilizar antes de alternar a proteção.
void atualizarBotao(unsigned long agora) {
  int leitura = digitalRead(botao);
  if (leitura != leituraBotao) {
    leituraBotao = leitura;
    mudouBotao = agora;
  }
  if (leituraBotao == botaoEstavel || agora - mudouBotao < 30) return;
  botaoEstavel = leituraBotao;
  if (botaoEstavel == LOW) {
    protecao = !protecao;
    alarme = false;
    noTone(buzzer);
  }
}

// O PIR acende o LED. Após o último sinal de movimento, a luz dura mais 3 s.
void atualizarRecepcao(unsigned long agora) {
  if (digitalRead(pir) == HIGH) {
    ultimoMovimento = agora;
    luz = true;
  }
  if (luz && agora - ultimoMovimento >= tempoLuz) luz = false;
  digitalWrite(led, luz ? HIGH : LOW);
}

// Mede a distância a cada 70 ms. Sem eco, a área é considerada livre.
void medirDistancia(unsigned long agora) {
  if (agora - ultimaMedicao < 70) return;
  ultimaMedicao = agora;
  digitalWrite(trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig, LOW);
  unsigned long pulso = pulseIn(echo, HIGH, 24000);
  perto = pulso > 0 && pulso <= limiteUs;
}

void loop() {
  unsigned long agora = millis();
  atualizarBotao(agora);
  atualizarRecepcao(agora);
  medirDistancia(agora);

  // O botão controla a proteção da obra. A recepção continua funcionando.
  if (!protecao) {
    mostrar(7, "Mona Lisa", "Protecao off");
    return;
  }
  // Ao afastar o objeto, o buzzer para e o LCD volta à tela de repouso.
  if (!perto) {
    if (alarme) {
      noTone(buzzer);
      alarme = false;
    }
    mostrar(6, "Mona Lisa", "Obra protegida");
    return;
  }

  // O buzzer começa a tocar assim que o sensor detecta algo dentro do limite.
  if (alarme) return;
  alarme = true;
  tone(buzzer, 1800);
  mostrar(0, "Alarme acionado", "");
}
