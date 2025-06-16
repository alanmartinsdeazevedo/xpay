from flask import Flask, request, jsonify
from flask_cors import CORS
from threading import Thread
from .app_config import config, env
from .app_coleta import ExecuteColeta, ObterLogs
from .app_mensagens import mensagem
import datetime

app = Flask(__name__)
CORS(app)


@app.route('/healthcheck', methods=['GET'])
def HealthCheck():
    response = {
        'status': 0,
        'mensagem': 'API de Integração com o Scope',        
    }
    return response


@app.route('/payment', methods=['POST'])
def PaymentPost():
    try:
        obj = request.get_json()
        id = obj['id']
        api_id = datetime.datetime.now().strftime("%Y%m%d%H%M%S")
        response = {
            'id': id,
            'api_id': api_id,
            'status': True,
            'mensagem': mensagem['3']
        }

        thread = Thread(target=ExecuteColeta, kwargs={'obj': obj, 'id': id, 'api_id': api_id})
        thread.start()
    except:
        response = {
            'status': False
        }
        
    return jsonify(response)


@app.route('/payment', methods=['GET'])
def PaymentGet():
    response = ObterLogs(request)    
    return jsonify(response)


if __name__ == '__main__':
    app.run(host=config[env]['host'], port=config[env]['port'], debug=True)
