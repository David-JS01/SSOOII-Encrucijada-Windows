

#include <windows.h>
#include <stdio.h>
#include "cruce2.h"
#include <stdlib.h>

void perrorExit(const char* mensaje, int retorno);
void sigInt(int signo);
BOOL WINAPI CtrlHandler(DWORD fdwCtrlType) {
    switch (fdwCtrlType)
    {
        
    case CTRL_C_EVENT:
        sigInt(1);
        return TRUE;
        
    case CTRL_CLOSE_EVENT:
        printf("Ctrl-Close event\n\n");
        return TRUE;

        
    case CTRL_BREAK_EVENT:
        printf("Ctrl-Break event\n\n");
        return FALSE;

    case CTRL_LOGOFF_EVENT:
        printf("Ctrl-Logoff event\n\n");
        return FALSE;

    case CTRL_SHUTDOWN_EVENT:
        printf("Ctrl-Shutdown event\n\n");
        return FALSE;

    default:
        return FALSE;
    }
}


DWORD WINAPI gestorSemaforico(void *argumento);
DWORD WINAPI funcionHijo(LPVOID proceso);

HINSTANCE libreria = LoadLibrary("cruce2.dll");

FARPROC CRUCE_fin = GetProcAddress(libreria, "CRUCE_fin");
FARPROC CRUCE_gestor_inicio = GetProcAddress(libreria, "CRUCE_gestor_inicio");
FARPROC CRUCE_nuevo_proceso = GetProcAddress(libreria, "CRUCE_nuevo_proceso");
FARPROC CRUCE_fin_coche = GetProcAddress(libreria, "CRUCE_fin_coche");
FARPROC CRUCE_fin_peatOn = GetProcAddress(libreria, "CRUCE_fin_peatOn");
FARPROC pausa = GetProcAddress(libreria, "pausa");
FARPROC pausa_coche = GetProcAddress(libreria, "pausa_coche");
FARPROC refrescar = GetProcAddress(libreria, "refrescar");

INT(*CRUCE_inicio) (int, int);
INT(*CRUCE_pon_semAforo) (int, int);
struct posiciOn(*CRUCE_avanzar_coche) (struct posiciOn);
struct posiciOn(*CRUCE_inicio_coche) (void);
struct posiciOn(*CRUCE_avanzar_peatOn) (struct posiciOn);
struct posiciOn(*CRUCE_nuevo_inicio_peatOn) (void);
VOID (*pon_error) (const char *);

int numeroProcesos;
int cochesCruzando1=0, cochesCruzando2=0, peatonesCruzando1=0, peatonesCruzando2=0;
HANDLE semaforoPadreCrear;
HANDLE posicionCoches[58];
HANDLE posicionPeaton[867];
HANDLE semaforo_peatones_1; 
HANDLE semaforo_peatones_2; 
HANDLE semaforo_coches_1;
HANDLE semaforo_coches_2;
HANDLE semaforo_cruce;
HANDLE coche_cruzando_1;
HANDLE coche_cruzando_2;
HANDLE peaton_cruzando_1;
HANDLE peaton_cruzando_2;
HANDLE cruzandoPaso1[15];
HANDLE cruzandoPaso2[21];
HANDLE hijos[49];

int main(int argc, char *argv[])
{
    
    int i;
    int var;
    SetConsoleCtrlHandler(CtrlHandler, TRUE);
    int velocidad, proceso, continua=1;
    if (libreria == NULL) {
        perrorExit("error al cargar libreria", 1);
    }
    if (CRUCE_fin == NULL) {
        perrorExit("Error al abrir funcion de dll", 50);
    }
    if (CRUCE_gestor_inicio == NULL) {
        perrorExit("Error al abrir funcion de dll", 51);
    }
    if (CRUCE_nuevo_proceso == NULL) {
        perrorExit("Error al abrir funcion de dll", 52);
    }
    if (CRUCE_fin_coche == NULL) {
        perrorExit("Error al abrir funcion de dll", 53);
    }
    if (CRUCE_fin_peatOn == NULL) {
        perrorExit("Error al abrir funcion de dll", 54);
    }
    if (pausa == NULL) {
        perrorExit("Error al abrir funcion de dll", 55);
    }
    if (pausa_coche == NULL) {
        perrorExit("Error al abrir funcion de dll", 56);
    }
    if (refrescar == NULL) {
        perrorExit("Error al abrir funcion de dll", 57);
    }

    CRUCE_inicio = (INT(*)(int, int))GetProcAddress(libreria, "CRUCE_inicio");
    if (CRUCE_inicio == NULL) {
        perrorExit("Error al abrir funcion de dll", 58);
    }
    CRUCE_pon_semAforo = (INT(*)(int, int))GetProcAddress(libreria, "CRUCE_pon_semAforo");
    if (CRUCE_pon_semAforo == NULL) {
        perrorExit("Error al abrir funcion de dll", 59);
    }
    CRUCE_avanzar_coche = (struct posiciOn(*)(struct posiciOn))GetProcAddress(libreria, "CRUCE_avanzar_coche");
    if (CRUCE_avanzar_coche == NULL) {
        perrorExit("Error al abrir funcion de dll", 60);
    }
    CRUCE_inicio_coche = (struct posiciOn(*)(void))GetProcAddress(libreria, "CRUCE_inicio_coche");
    if (CRUCE_inicio_coche == NULL) {
        perrorExit("Error al abrir funcion de dll", 61);
    }
    CRUCE_avanzar_peatOn = (struct posiciOn(*)(struct posiciOn))GetProcAddress(libreria, "CRUCE_avanzar_peatOn");
    if (CRUCE_avanzar_peatOn == NULL) {
        perrorExit("Error al abrir funcion de dll", 62);
    }
    CRUCE_nuevo_inicio_peatOn = (struct posiciOn(*)(void))GetProcAddress(libreria, "CRUCE_nuevo_inicio_peatOn");
    if (CRUCE_nuevo_inicio_peatOn == NULL) {
        perrorExit("Error al abrir funcion de dll", 63);
    }
    pon_error = (VOID(*)(const char *))GetProcAddress(libreria, "pon_error");
    if (pon_error == NULL) {
        perrorExit("Error al abrir funcion de dll", 64);
    }

    if (argc != 3) {
        perrorExit("Debe incluir dos argumentos la ejecucion del programa", 1);
    }

    numeroProcesos = atoi(argv[1]);
    velocidad = atoi(argv[2]);

    if (numeroProcesos <= 2) {
        perrorExit("Debe haber mas de dos procesos", 2);
    }

    if (velocidad < 0) {
        perrorExit("la velocidad debe ser mayor que 0", 2);
    }

    for (i = 0; i < 867; i++) {
        posicionPeaton[i] = CreateMutex(NULL, FALSE, NULL);
        if (posicionPeaton[i] == NULL) {
            perrorExit("Error creacion mutex", 3);
        }
    }

    for (i = 1; i <= 57; i++) {
        posicionCoches[i] = CreateMutex(NULL, FALSE, NULL);
        if (posicionCoches[i] == NULL) {
            perrorExit("Error creacion mutex", 4);
        }
    }
    semaforo_peatones_1 = CreateEvent(NULL, TRUE, TRUE, "evento control semaforo peatones 1");
    if (semaforo_peatones_1 == NULL) {
        perrorExit("Error creacion evento semaforo 1", 5);
    }
    semaforo_peatones_2 = CreateEvent(NULL, TRUE, FALSE, "evento control semaforo peatones 2");
    if (semaforo_peatones_2 == NULL) {
        perrorExit("Error creacion evento semaforo 2", 6);
    }

    semaforo_coches_1 = CreateSemaphore(NULL, 0, 1, NULL);
    if (semaforo_coches_1 == NULL) {
        perrorExit("Error creacion semaforo coches 1", 7);
    }

    semaforo_coches_2 = CreateSemaphore(NULL, 0, 1, NULL);
    if (semaforo_coches_2 == NULL) {
        perrorExit("Error creacion semaforo coches 2", 8);
    }

    semaforo_cruce = CreateSemaphore(NULL, 1, 1, NULL);
    if (semaforo_cruce == NULL) {
        perrorExit("Error creacion semaforo cruce", 9);
    }

    HANDLE memoria = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(continua), "memoriaCompartida1");
    if (memoria == NULL) {
        perrorExit("Error creacion archivo de memoria compartida", 10);
    }

    LPVOID puntero = (LPVOID)MapViewOfFile(memoria, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(continua));
    if (puntero == NULL) {
        perrorExit("Error mapeo de memoria", 11);
    }

    CopyMemory(puntero, &continua, sizeof(continua));

    var = 0;
    for (i = 696; i <= 802; i++) {
        cruzandoPaso1[var] = posicionPeaton[i];
        var++;
        if (i == 700)
            i = 746;
        if (i == 751)
            i = 797;
    }
    var = 0;
    for (i = 428; i <= 536; i++) {
        cruzandoPaso2[var] = posicionPeaton[i];
        var++;
        if (i == 434)
            i = 478;
        if (i == 485)
            i = 529;
    }


    
    
   
    
    if ((semaforoPadreCrear = CreateSemaphore(NULL, numeroProcesos-1, numeroProcesos-1, NULL)) == NULL) {
        perrorExit("Error creacion semaforo del padre", 12);
    }

    CRUCE_inicio(velocidad, numeroProcesos);
    
    WaitForSingleObject(semaforoPadreCrear, INFINITE);

    CreateThread(NULL, 0, gestorSemaforico, LPVOID(), 0, NULL);
    


    long var2 = 0;
    while (1) {
        proceso = CRUCE_nuevo_proceso();
        WaitForSingleObject(semaforoPadreCrear, INFINITE);
        hijos[var2%numeroProcesos]=CreateThread(NULL, 0, funcionHijo, LPVOID(proceso), 0, NULL);
        if (hijos[var2 % numeroProcesos] == NULL) {
            perrorExit("Error creacion hijos", 13);
        }
        var2++;
    }

    return 0;

}

void perrorExit(const char* mensaje, int retorno)
{
    PERROR(mensaje);
    exit(retorno);
}

void sigInt(int signo) {

    int i, continua=0;
    
    HANDLE memoria = OpenFileMapping(FILE_MAP_ALL_ACCESS, NULL, "memoriaCompartida1");
    if (memoria == NULL) {
        perrorExit("Error creacion archivo de memoria compartida", 14);
    }
    LPVOID puntero = (LPVOID)MapViewOfFile(memoria, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(int));
    if (puntero == NULL) {
        perrorExit("Error mapeo de memoria", 15);
    }
    CopyMemory(puntero, &continua, sizeof(continua));
    WaitForMultipleObjects(numeroProcesos, hijos, TRUE, INFINITE);
    CRUCE_fin();
    system("cls");
    FreeLibrary(libreria);
    UnmapViewOfFile(puntero);
    if (CloseHandle(memoria) == NULL) {
        perrorExit("Error al cerrar handle", 41);
    }
    if (CloseHandle(semaforoPadreCrear) == NULL) {
        perrorExit("Error al cerrar handle", 17);
    }
    if (CloseHandle(semaforo_peatones_1) == NULL) {
        perrorExit("Error al cerrar handle", 18);
    }
    if (CloseHandle(semaforo_peatones_2) == NULL) {
        perrorExit("Error al cerrar handle", 19);
    }
    if (CloseHandle(semaforo_coches_1) == NULL) {
        perrorExit("Error al cerrar handle", 20);
    }
    if (CloseHandle(semaforo_coches_2) == NULL) {
        perrorExit("Error al cerrar handle", 21);
    }
    if (CloseHandle(semaforo_cruce) == NULL) {
        perrorExit("Error al cerrar handle", 22);
    }
    for (i = 0; i < numeroProcesos; i++) {
        CloseHandle(hijos[i]);
    }
    for (i = 0; i < 867; i++) {
        if (CloseHandle(posicionPeaton[i]) == NULL) {
            perrorExit("Error al cerrar handle", 24);
        }
    }
    for (i = 1; i < 58; i++) {
        if (CloseHandle(posicionCoches[i]) == NULL) {
            perrorExit("Error al cerrar handle", 25);
        }
    }
    

    system("cls");
    printf("\nEl programa ha finalizado correctamente!\n");
    exit(0);

    

}

DWORD WINAPI gestorSemaforico( void *argumento) {
    int i;
    CRUCE_gestor_inicio();
    HANDLE memoria = OpenFileMapping(FILE_MAP_ALL_ACCESS, NULL, "memoriaCompartida1");
    if (memoria == NULL) {
        perrorExit("Error creacion archivo de memoria compartida", 28);
    }
    LPVOID puntero = (LPVOID)MapViewOfFile(memoria, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(int));
    if (puntero == NULL) {
        perrorExit("Error mapeo de memoria", 29);
    }
    while ((int)puntero) {
        CRUCE_pon_semAforo(SEM_C1, VERDE);
        ReleaseSemaphore(semaforo_coches_1, 1, NULL);
        CRUCE_pon_semAforo(SEM_C2, ROJO);
        ResetEvent(semaforo_peatones_1);
        CRUCE_pon_semAforo(SEM_P1, ROJO);
        CRUCE_pon_semAforo(SEM_P2, VERDE);
        SetEvent(semaforo_peatones_2);
        for (i = 0; i < 6; i++) {
            pausa();
            if (i == 4) {
                WaitForSingleObject(semaforo_coches_1, INFINITE);
                CRUCE_pon_semAforo(SEM_C1, AMARILLO);
            }
        }

        CRUCE_pon_semAforo(SEM_C1, ROJO);
        CRUCE_pon_semAforo(SEM_C2, VERDE);
        ReleaseSemaphore(semaforo_coches_2, 1, NULL);
        CRUCE_pon_semAforo(SEM_P1, ROJO);
        ResetEvent(semaforo_peatones_2);
        CRUCE_pon_semAforo(SEM_P2, ROJO);
        for (i = 0; i < 9; i++) {
            pausa();
            if (i == 7) {
                WaitForSingleObject(semaforo_coches_2, INFINITE);
                CRUCE_pon_semAforo(SEM_C2, AMARILLO);
            }
        }

        CRUCE_pon_semAforo(SEM_C1, ROJO);
        CRUCE_pon_semAforo(SEM_C2, ROJO);
        CRUCE_pon_semAforo(SEM_P1, VERDE);
        SetEvent(semaforo_peatones_1);
        CRUCE_pon_semAforo(SEM_P2, ROJO);
        for (i = 0; i < 12; i++)
            pausa();

    }
    UnmapViewOfFile(puntero);
    if (CloseHandle(memoria) == NULL) {
        perrorExit("Error al cerrar handle", 42);
    }
    return 0;
}

DWORD WINAPI funcionHijo(LPVOID proceso) {
   struct posiciOn pos, pos2, posAnterior;
   int i;
    int zonaDePeligro=0, tipoCoche;
    int tipo2, tipo3, tipoAnterior, tipoGuardado, posic;
    posAnterior.y = 0;
    posAnterior.x = 0;
    char mens[20];
    HANDLE esperar1[6];
    HANDLE esperar2[6];
    HANDLE esperar3[9];
    int var = 428;
    for (i = 0; i < 6; i++) {
        esperar1[i] = posicionPeaton[var++];
        if (var == 430 || var == 481)
            var += 49;
    }
    var = 430;
    for (i = 0; i < 6; i++) {
        esperar2[i] = posicionPeaton[var++];
        if (var == 432 || var == 483)
            var += 49;
    }
    var = 432;
    for (i = 0; i < 9; i++) {
        esperar3[i] = posicionPeaton[var++];
        if (var == 435 || var == 486)
            var += 48;
    }
    HANDLE memoria = OpenFileMapping(FILE_MAP_ALL_ACCESS, NULL, "memoriaCompartida1");
    if (memoria == NULL) {
        perrorExit("Error creacion archivo de memoria compartida", 31);
    }
    LPVOID puntero = (LPVOID)MapViewOfFile(memoria, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(int));
    if (puntero == NULL) {
        perrorExit("Error mapeo de memoria", 32);
    }
    if ((int)proceso == PEAToN) {
        zonaDePeligro = 1;
        while (1) {
            pos = CRUCE_nuevo_inicio_peatOn();
            tipo2 = pos.y * 50 + pos.y + pos.x;
            tipoAnterior = tipo2;
            WaitForSingleObject(posicionPeaton[tipo2], INFINITE); 
            pos = CRUCE_avanzar_peatOn(pos);
            pausa();
            while ((int)puntero && pos.y >= 0) {
                tipo2 = pos.y * 50 + pos.y + pos.x;
                WaitForSingleObject(posicionPeaton[tipo2], INFINITE);


             

                if (tipo2 == 693 || tipo2 == 744 || tipo2 == 795) {
                    WaitForSingleObject(semaforo_peatones_1, INFINITE);
                }
                if (tipo2 >= 582 && tipo2 <= 589) {
                    WaitForSingleObject(semaforo_peatones_2, INFINITE);
                }

                pos = CRUCE_avanzar_peatOn(pos);
                pausa();

                ReleaseMutex(posicionPeaton[tipoAnterior]);
                tipoAnterior = tipo2;

                

            }
            ReleaseMutex(posicionPeaton[tipoAnterior]);
            CRUCE_fin_peatOn();
            ReleaseSemaphore(semaforoPadreCrear, 1, NULL);
            UnmapViewOfFile(puntero);
            if (CloseHandle(memoria) == NULL) {
                perrorExit("Error al cerrar handle", 43);
            }
            
            return 0;
        }
    }
    if ((int)proceso == COCHE) {
        while (1) {
            pos=CRUCE_inicio_coche();
            if (pos.x == -3)
                tipoCoche = 0;
            else if (pos.y == 1)
                tipoCoche = 1;
            while ((int)puntero && pos.y >= 0) { 
                if (pos.y == 10)
                    tipo2 = pos.x + 4;
                else if (pos.x == 33)
                    tipo2 = pos.y + 37;
                
                if (posAnterior.x == 33 && tipo2 == 37) {
                    tipo2 = 47;
                }
                tipoGuardado = tipo2;
                if (tipo2 == 1) {
                    WaitForSingleObject(posicionCoches[tipo2], INFINITE);
                }
                else if (tipo2 < 38) {
                    WaitForSingleObject(posicionCoches[tipo2], INFINITE);
                    WaitForSingleObject(posicionCoches[tipo2-1], INFINITE);
                }

                if (tipo2 >= 38) {
                    tipo3 = tipo2;
                    if (tipo2 == 47)
                        tipo3 = 37;
                    if (tipo2 == 49 && posAnterior.y == 10) {
                        tipo3 = 46;
                        WaitForSingleObject(posicionCoches[tipo3], INFINITE);
                        tipo3 = 48;
                        WaitForSingleObject(posicionCoches[tipo3], INFINITE);
                        tipo3 = tipo2;
                    }
                    WaitForSingleObject(posicionCoches[tipo3], INFINITE);
                }

                tipo2 = tipoGuardado;
                if (tipo2 == 45 || tipo2 == 31) {
                    WaitForSingleObject(semaforo_cruce, INFINITE);
                }

                if (tipo2 == 19 || tipo2 == 20) {
                    WaitForMultipleObjects(6, esperar1, TRUE, INFINITE);
                }
                if (tipo2 == 21 || tipo2 == 22) {
                    WaitForMultipleObjects(6, esperar2, TRUE, INFINITE);
                }
                if (tipo2 == 23 || tipo2 == 24) {
                    WaitForMultipleObjects(9, esperar3, TRUE, INFINITE);
                }

                if (tipo2 == 50) {
                    WaitForMultipleObjects(15, cruzandoPaso1, TRUE, INFINITE);
                }
                
                if (pos.x == 33 && pos.y == 6) {
                    WaitForSingleObject(semaforo_coches_1, INFINITE);
                }
                if (pos.x == 13 && pos.y == 10) {
                    WaitForSingleObject(semaforo_coches_2, INFINITE);
                }
                pos2 = CRUCE_avanzar_coche(pos);
                if (pos.x == 33 && pos.y == 6) {
                    ReleaseSemaphore(semaforo_coches_1, 1, NULL);
                }
                if (pos.x == 13 && pos.y == 10) {
                    ReleaseSemaphore(semaforo_coches_2, 1, NULL);
                }
                pausa_coche();
                
                if (tipo2 == 53) {
                    ReleaseSemaphore(semaforo_cruce, 1, NULL);
                }
                if (tipo2 == 31) {
                    for (i = 0; i < 6; i++) {
                        ReleaseMutex(esperar1[i]);
                    }
                }
                if (tipo2 == 33) {
                    for (i = 0; i < 6; i++) {
                        ReleaseMutex(esperar2[i]);
                    }
                }
                if (tipo2 == 35 ) {
                    for (i = 0; i < 9; i++) {
                        ReleaseMutex(esperar3[i]);

                    }
                }

                if (tipo2 == 57) {
                    for (i = 0; i < 15; i++) {
                        ReleaseMutex(cruzandoPaso1[i]);

                    }
                    
                }

                if (tipo2 > 7 && tipo2 < 38) {
                    posic = tipo2 - 7;
                    ReleaseMutex(posicionCoches[posic]);
                    posic = tipo2 - 8;
                    ReleaseMutex(posicionCoches[posic]);
                }

                if (tipo2 > 41) {
                    if (posAnterior.y == 10 && tipoCoche == 0) {
                        for (i = 31; i <= 36; i++) {
                            ReleaseMutex(posicionCoches[i]);
                        }
                    }
                    else {
                        posic = tipo2 - 4;
                        if (tipo2 == 51)
                            posic = 37;
                        ReleaseMutex(posicionCoches[posic]);
                    }
                }

                posAnterior = pos;
                pos = pos2;
            }
            CRUCE_fin_coche();
            for (i = 54; i <= 57; i++) {
                ReleaseMutex(posicionCoches[i]);
            }
            ReleaseSemaphore(semaforoPadreCrear, 1, NULL);
            UnmapViewOfFile(puntero);
            if (CloseHandle(memoria) == NULL) {
                perrorExit("Error al cerrar handle", 44);
            }
            
            return 0;
        }
    }
    return 0;

}
