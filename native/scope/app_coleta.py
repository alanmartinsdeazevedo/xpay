import time
import shutil
import json
from os import walk
from pywinauto.application import Application
from pywinauto import Desktop
from .app_config import config, env
from .app_codigos import bandeira, servico, rede
from .app_mensagens import mensagem
from .app_scope import FecharSessao


def ExecuteColeta(obj, id, api_id):
    # inicializacao
    dlg = None
    log = f'.\\log\\{api_id}-{id}.txt'
    forma_pgto = obj['forma_de_pagamento']

    if forma_pgto == 'credito' or forma_pgto == 'debito':
        try:
            FecharSessao()
            GravarLog(api_id, id, {}, 'inicio')
            dlg = InicializaAplicacao('', 'inicio')
            status = 0
        except:
            status = 90003

        if status == 0:
            status = InicializaScope(dlg, 'inicio')

            if status == 0:
                if forma_pgto == 'credito':
                    OperacaoCredito(obj, dlg)
                    SimularCredito(dlg)
                else:
                    OperacaoDebito(obj, dlg)
                        
                # aguarda pagamento
                status = AguardarPagamento(dlg)

                if status == 0:
                    # finalizar operacao
                    FinalizarPgto(dlg)
                    controle = ObterControle()
                    FinalizarOperacao(dlg)

                    # abertura de nova sessao para obter os dados da transacao
                    status = ObterDadosTransacao(controle, log)

                    if status == 0:
                        # trata dados de retorno
                        transacao = FormatarDados(log, controle)
                        FormatarRetorno(api_id, id, transacao, status)
        
        if status != 0:
            InterromperOperacao(dlg, forma_pgto)
            FormatarRetorno(api_id, id, {}, status)
    else:
        FormatarRetorno(api_id, id, {}, '90000')


def InicializaAplicacao(log, acao):
    if acao == 'inicio':
        LimparTrace()

    if log == '':
        cmd = config[env]['path_coleta']
    else:
        cmd = config[env]['path_bat'] + ' ' + log

    app = Application().start(f'{cmd}', create_new_console=True, wait_for_idle=False, timeout=120)
    dlg = app.top_window()

    return dlg


def InicializaScope(dlg, acao):
    if acao == 'final':
        empr = config[env]['empresa']
        fil = config[env]['filial']
        pdv = config[env]['pdv']
        
        # insere empresa
        time.sleep(0.3)
        dlg.type_keys(f'{empr}' + '{' + 'ENTER' + '}')

        # insere filial
        dlg.type_keys(f'{fil}' + '{' + 'ENTER' + '}')

        # insere pdv
        dlg.type_keys(f'{pdv}' + '{' + 'ENTER' + '}')

    contador = 0
    while True:
        status = AguardarServidor(contador)
        contador += 1
        if status >= 0:
            break

    return status


def AguardarServidor(contador):
    time.sleep(config[env]['sleep_small'])
    retorno = -1
    with open(config[env]['path_trace']) as f:
        content = f.read()
        if 'ScopeAbreSessaoTEF -> {Retorno = 0' in content:
            retorno = 0
            time.sleep(config[env]['sleep_medium'])
        else:
            if contador > config[env]['timeout_server']:
                retorno = 90004

    return retorno


def FinalizarOperacao(dlg):
    time.sleep(config[env]['sleep_medium'])
    dlg.type_keys('00' + '{' + 'ENTER' + '}')
    time.sleep(config[env]['sleep_large'])


def OperacaoCredito(obj, dlg):
    vlr = obj['valor']
    cmdstring = '01' + '{' + 'ENTER' + '}' + vlr + '{' + 'ENTER' + '}'
    dlg.type_keys(cmdstring)


def OperacaoDebito(obj, dlg):
    vlr = obj['valor']
    dlg.type_keys('02' + '{' + 'ENTER' + '}' + vlr + '{' + 'ENTER' + '}')


def AguardarPagamento(dlg):
    contador = 0
    avista = 0
    while True:
        time.sleep(config[env]['sleep_medium'])
        retorno = -1
        with open(config[env]['path_trace']) as f:
            content = f.read()
            if 'lpParam->MsgOp2 = A vista?' in content and avista == 0:
                dlg.type_keys('1' + '{' + 'ENTER' + '}p')
                avista = 1
            if 'Refaca a transacao' in content:
                dlg.type_keys('c' + '{' + 'ENTER' + '}')
                retorno = 90006
            if 'RETIRE O CARTAO' in content:
                contador = 0
            if 'Imprimindo...' in content:
                retorno = 0
            else:
                if contador > config[env]['timeout_pagamento']:
                    retorno = 90001
        contador += 1
        if retorno >= 0:
            break

    return retorno


def InterromperOperacao(dlg, forma_pgto):
    print('Interrompeu Operacao')
    try:
        time.sleep(config[env]['sleep_small'])
        dlg.type_keys('^c')
        #if forma_pgto == 'debito':
        dlg.type_keys('^c')
    except:
        print('excecao no momento do control-c')
        pass

    try:
        time.sleep(config[env]['sleep_large'])
        windows = Desktop(backend="uia").windows()
        if 'tstcoleta.exe' in [w.window_text() for w in windows]:
            dlg.type_keys('^c')
    except:
        print('excecao ao matar a sessao')
        pass


def SimularCredito(dlg):
    if config[env]['simular']:
        # inserir cartao de credito
        time.sleep(config[env]['sleep_small'])
        dlg.type_keys('4567930933187002' + '{' + 'ENTER' + '}p')

        # data de validade
        time.sleep(config[env]['sleep_small'])
        dlg.type_keys('0124' + '{' + 'ENTER' + '}p')

        # codigo de seguranca
        time.sleep(config[env]['sleep_small'])
        dlg.type_keys('111' + '{' + 'ENTER' + '}p')

        # parcelamento
        time.sleep(config[env]['sleep_small'])
        dlg.type_keys('1' + '{' + 'ENTER' + '}p')
    else:
        pass

def FinalizarPgto(dlg):
    # cabecalho
    time.sleep(config[env]['sleep_small'])
    dlg.type_keys('{' + 'ENTER' + '}' + '{' + 'ENTER' + '}')

    # demonstrativo
    time.sleep(config[env]['sleep_small'])
    dlg.type_keys('{' + 'ENTER' + '}' + '{' + 'ENTER' + '}' + '{' + 'ENTER' + '}')

    # cupom reduzido
    time.sleep(config[env]['sleep_small'])
    #dlg.type_keys('{' + 'ENTER' + '}' + '{' + 'ENTER' + '}' + 'pc' +  '{' + 'ENTER' + '}') #coleta.exe
    dlg.type_keys('{' + 'ENTER' + '}' + 'p' + '{' + 'ENTER' + '}') #tstcoleta.exe


def ObterControle():
    controle = '0'
    while True:
        time.sleep(config[env]['sleep_small'])
        with open(config[env]['path_trace']) as f:
            content = f.read()
            if 'lpParam->MsgOp2 = Controle ' in content:
                controle = content.split('lpParam->MsgOp2 = Controle ')[1][:11]
                break
            else:
                controle = ''

    return controle


def ObterDadosTransacao(controle, log):
    try:
        time.sleep(config[env]['sleep_extra'])
        dlg = InicializaAplicacao(log, 'final')
        ret = InicializaScope(dlg, 'final')

        # opcao
        time.sleep(config[env]['sleep_small'])
        dlg.type_keys('09' + '{' + 'ENTER' + '}')

        # controle
        time.sleep(config[env]['sleep_small'])
        dlg.type_keys(controle + '{' + 'ENTER' + '}')

        # visualizacao
        time.sleep(config[env]['sleep_medium'])
        dlg.type_keys('{' + 'ENTER' + '}' + 'n')

        FinalizarOperacao(dlg)
    except:
        ret = 90005

    return ret


def FormatarDados(file, controle):
    try:
        contador = 0
        while True:
            time.sleep(config[env]['sleep_small'])
            with open(file) as f:
                content = f.read()
                if 'Codigo Autorizacao' in content or contador >= config[env]['timeout_pagamento']:
                    break
            contador += 1
        
        with open(file) as f:
            content = f.read()
            try:
                value = content.split('Codigo Autorizacao                          = [')[1]
                index = value.find(']')
                autorizacao = value[:index]
            except:
                autorizacao = ''
            try:
                value = content.split('Codigo do Servico                           = [')[1]
                index = value.find(']')
                codservico = value[:index]
            except:
                codservico = '0'
            try:
                value = content.split('Valor                                       = [')[1]
                index = value.find(']')
                vlr_transacao = value[:index]
            except:
                vlr_transacao = ''
            try:
                value = content.split('Numero de Parcelas                          = [')[1]
                index = value.find(']')
                parc_transacao = value[:index]
            except:
                parc_transacao = ''
            try:
                value = content.split('Bandeira                                    = [')[1]
                index = value.find(']')
                codbandeira = value[:index]
            except:
                codbandeira = '0'
            try:
                value = content.split('Mensagem de Status                          = [')[1]
                index = value.find(']')
                scopestatus = value[:index]
            except:
                scopestatus = ''
            try:
                value = content.split('Mensagem da Situacao                        = [')[1]
                index = value.find(']')
                scopesituacao = value[:index]
            except:
                scopesituacao = '0'
            try:
                value = content.split('Codigo Estabelecimento                      = [')[1]
                index = value.find(']')
                codestabelecimento = value[:index]
            except:
                codestabelecimento = '0'
            try:
                value = content.split('Rede                                        = [')[1]
                index = value.find(']')
                codrede = value[:index]
            except:
                codrede = '0'
            try:
                value = content.split('NSU                                         = [')[1]
                index = value.find(']')
                nsu = value[:index]
            except:
                nsu = '0'
            try:
                value = content.split('NSU do Host                                 = [')[1]
                index = value.find(']')
                nsuhost = value[:index]
            except:
                nsuhost = '0'

        cnpj = ObterCNPJ(file)
        
        retorno = {
            'empresa': config[env]['empresa'],
            'filial': config[env]['filial'],
            'pdv': config[env]['pdv'],
            'nsu': nsu,
            'nsu_host': nsuhost,
            'autorizacao': autorizacao,
            'cupom': '',
            'cnpj': cnpj,
            'codigo_controle': controle,
            'codigo_forma_pagamento': codservico,
            'forma_pagamento': servico[codservico],
            'valor_transacao': vlr_transacao,
            'parcelas_transacao': parc_transacao,
            'codigo_bandeira': codbandeira,
            'nome_bandeira': bandeira[codbandeira],
            'codigo_rede': codrede,
            'nome_rede': rede[codrede],
            'codigo_estabelecimento': codestabelecimento,
            'scope_status': scopestatus,
            'scope_situacao': scopesituacao
        }
    except:
        retorno = {}

    return retorno


def ObterCNPJ(file):
    cnpj = ''
    try:
        with open(config[env]['path_trace']) as file:
            getCnpj = False
            for line in file:
                if getCnpj:
                    cnpj = line.rstrip()
                    getCnpj = False
                if 'ScopeGetCupomEx -> Cabec' in line.rstrip():
                    getCnpj = True
    except:
        pass

    return cnpj


def FormatarRetorno(api_id, id, transacao, retorno):
    try:
        msg = mensagem[str(retorno)]
    except:
        msg = ''

    if retorno > 3:
        retorno = 2

    try:
        transacao['id'] = id
        transacao['api_id'] = api_id
        transacao['status'] = retorno
        transacao['mensagem'] = msg

        GravarLog(api_id, id, transacao, 'final')
        GravarTrace(api_id, id)
    except:
        transacao = {
            'id': id,
            'api_id': api_id,
            'status': 2,
            'mensagem': mensagem['90002']
        }
        GravarLog(api_id, id, transacao, 'final')

    FecharSessao()


def GravarLog(api_id, id, transacao, acao):
    log = f'.\\log\\{api_id}-{id}.json'

    if acao == 'inicio':
        transacao = {
            'id': id,
            'api_id': api_id,
            'status': 1,
            'mensagem': mensagem['1']
        }
        f = open(log, 'a')
        f.write(str(transacao))
        f.close()
    else:
        with open(log,'w') as f:
            pass

        f = open(log, 'w')
        f.write(str(transacao))
        f.close()


def GravarTrace(api_id, id):
    time.sleep(config[env]['sleep_large'])
    destino = f'.\\log\\{api_id}-{id}.log'
    shutil.copyfile(config[env]['path_trace'], destino)


def LimparTrace():
    time.sleep(config[env]['sleep_large'])
    with open(config[env]['path_trace'],'w') as f:
        pass


def ObterLogs(request):
    response = []
    filenames = next(walk('.\\log'), (None, None, []))[2]

    try:
        if request.args.get('mes'):
            for f in filenames:
                if request.args.get('mes') == f[:6] and '.json' in f:
                    with open('.\\log\\'+f) as content:
                        response.append(json.loads(content.read().replace("'", '"')))
                    content.close()
        
        if request.args.get('dia'):
            for f in filenames:
                if request.args.get('dia') == f[:8] and '.json' in f:
                    with open('.\\log\\'+f) as content:
                        response.append(json.loads(content.read().replace("'", '"')))
                    content.close()
        
        if request.args.get('id'):
            for f in filenames:
                if request.args.get('id') == f.split('-')[1].replace('.json', '') and '.json' in f:
                    with open('.\\log\\'+f) as content:
                        response = json.loads(content.read().replace("'", '"'))
                    content.close()
        
        if request.args.get('apiid'):
            for f in filenames:
                if request.args.get('apiid') == f.split('-')[0] and '.json' in f:
                    with open('.\\log\\'+f) as content:
                        response = json.loads(content.read().replace("'", '"'))
                    content.close()
    except:
        pass

    return response