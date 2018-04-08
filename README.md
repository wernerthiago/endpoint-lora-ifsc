# Código para instalação de um Endpoint LoRaWAN + GPS

## Cuidados que devem ser tomados:
* Verificar o arquivo que está dentro da biblioteca LMIC chamado config.h. Nele precisamos setar o modelo US;
* As chaves de segurança são obtidas através do LoRa Server. Ao editar ou adicionar um Endpoint (Application) ir na tab ABP (Activate Device) e gerar as chaves. Por conveniência utilizamos a mesma chave para a Network Key e Application Key;
* Na mesma tab, lembrar de marcar a checkbox Disable Frame-Counter Activation;
* A pinagem que deve ser usada para o funcionamento do endpoint deve ser esta:
```
const lmic_pinmap lmic_pins = {
    .nss = 10,
    .rxtx = LMIC_UNUSED_PIN,
    .rst = 8,
    .dio = {2, 3, 4},
};
```
