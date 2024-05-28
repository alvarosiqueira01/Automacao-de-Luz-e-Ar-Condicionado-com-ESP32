# Automacao-de-Luz-e-Ar-Condicionado-com-ESP32
Projeto do trainee da Célula IoT - UFC

O presente projeto é uma aplicação de automação residencial usando o microcontrolador ESP32. Uma montagem dos circuitos pode ser encontrada em: https://wokwi.com/projects/399153215445408769

Seu funcionamento básico envolve o acionamento tanto automático via programação quanto manual via assinaturas MQTT de um sistema de iluminação e refrigeração de um cõmodo de uma residễncia, por exemplo. A ESP32 recebe informações de temperatura por meio de um sensor e a partir delas aciona automaticamente um relé caso a temperatura exceda um valor específico e o desliga caso a leitura fique abaixo de um outro valor específico. Recebe também informações de data e hora via servidor NTP que usa para acionar e desligar tanto o relé da iluminação quanto o relé da refrigeração conforme horários programados. 
Como dito, estes relés também podem ser controlados manualmente por assinaturas MQTT enviadas à ESP, podendo então receber comandos para ligar/desligar tanto luzes quanto ar condicionado, além de programar as variáveis de horário e temperatura que controlam o acionamento automático dos relés. 

