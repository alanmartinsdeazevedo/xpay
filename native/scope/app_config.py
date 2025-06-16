env = 'prod' # test or prod
emp = 'atibaia' # test or multiplay or cabo


totem = {
    'atibaia': {
        'empresa': '1121',
        'filial': '0001',
        'pdv': '008'
    },
    'test': {
        'empresa': '0001',
        'filial': '0001',
        'pdv': '043'
    }
}


config = {
    'prod': {
        'host': '0.0.0.0',
        'port': 5000,
        'empresa': totem[emp]['empresa'],
        'filial': totem[emp]['filial'],
        'pdv': totem[emp]['pdv'],
        'interface': 'exe',
        'modo_operacao': '2',
        'timeout_server': 80,
        'timeout_pagamento': 120,
        'sleep_small': 0.3,
        'sleep_medium': 0.5,
        'sleep_large': 1,
        'sleep_extra': 3,
        'simular': False,
        'path_coleta': '.\\tstcoleta.exe ' + totem[emp]['empresa'] + ' ' + totem[emp]['filial'] + ' ' + totem[emp]['pdv'] + ' ' + ' -d -m',
        'path_bat': '.\\coleta.bat',
        'path_trace': '.\\sc\\ScopeTrc.txt',
        'path_log': '.\\log\\log.txt',
        'path_dll': 'c:\\ScopePaymentApi\\scopeapi.dll'
    },
    'test': {
        'host': '0.0.0.0',
        'port': 5000,
        'empresa': totem[emp]['empresa'],
        'filial': totem[emp]['filial'],
        'pdv': totem[emp]['pdv'],
        'interface': 'exe',
        'modo_operacao': '2',
        'timeout_server': 80,
        'timeout_pagamento': 120,
        'sleep_small': 0.3,
        'sleep_medium': 0.5,
        'sleep_large': 1,
        'sleep_extra': 3,
        'simular': True,
        'path_coleta': '.\\tstcoleta.exe ' + totem[emp]['empresa'] + ' ' + totem[emp]['filial'] + ' ' + totem[emp]['pdv'] + ' ' + ' -d -m',
        'path_bat': '.\\coleta.bat',
        'path_trace': '.\\sc\\ScopeTrc.txt',
        'path_log': '.\\log\\log.txt',
        'path_dll': 'scopeapi.dll'
    }
}