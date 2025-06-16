import ctypes
from .app_config import config, env
Scope = ctypes.WinDLL(config[env]['path_dll'])


def FecharSessao():
    #desfez = ctypes.c_buffer(512)
    #retorno = Scope.ScopeFechaSessaoTEF(1, desfez) # 0 desfaz 1 confirma
    #print(f'Informacao: Finalizando Sessao TEF. Codigo de Retorno: {retorno}')

    retorno = Scope.ScopePPClose()
    print(f'Informacao: Finalizando Pinpad. Codigo de Retorno: {retorno}')

    retorno = Scope.ScopeClose()
    print(f'Informacao: Finalizando Scope Server. Codigo de Retorno: {retorno}')
