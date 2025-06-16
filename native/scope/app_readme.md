# Preparacao das bibliotecas python

OBS: Os passos abaixo são necessários caso necessite montar um novo ambiente do zero.
     Mas caso já tenha a pasta do projeto distribuida, não é necessário.

1 - Versao embarcada do python
    https://www.python.org/ftp/python/3.8.10/python-3.8.10-embed-win32.zip

2 - Adicionar o zip do passo anterior na pasta python

3 - Adicionar o arquivo get-pip.py na pasta python

4 - Editar o arquivo \python\python38._pth descomentando a linha do import site

5 - Instalar o PIP
    Executar o comando : .\python\python.exe .\python\get-pip.py

6 - Instalar o Flask
    Executar o comando : .\python\python.exe -m pip install flask

7 - Instalar o flask-cors
    Executar o comando : .\python\python.exe -m pip install flask-cors

8 - Instalar o pywinauto
    Executar o comando : .\python\python.exe -m pip install pywinauto

# Executar API em modo debug

.\python\python.exe -m flask --debug run --host=0.0.0.0

# Executar API

.\python\python.exe -m flask run --host=0.0.0.0

# Consumo da API

- Healthcheck
Metodo: GET
Endpoint: http://localhost:5000/healthcheck
Response:
{
	"descricao": "API de Integração com o Scope",
	"status": 0
}

- Payment
Metodo: POST
Endpoint: http://localhost:5000/payment
Body:
{
    "forma_de_pagamento": "credito",
    "valor": "200"
}
Response:
{
    "autorizacao": "000000", 
    "cupom': "", 
    "codigo_controle": "01944713019",
    "codigo_forma_pagamento": "009", 
    "forma_pagamento": "Compra com cartão de crédito á vista", 
    "valor_transacao": "005", 
    "parcelas_transacao': "1", 
    "codigo_bandeira": "001", 
    "nome_bandeira": "Visa", 
    "scope_status": "82", 
    "scope_situacao": "79", 
    "id": "20230119123823", 
    "status': "0", 
    "mensagem": "Operação executada com sucesso"
}