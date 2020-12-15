#include <iostream>
#include <cassert>
#include <thread>
#include <mutex>
#include <random> // dispositivos, generadores y distribuciones aleatorias
#include <chrono> // duraciones (duration), unidades de tiempo
#include "Semaphore.h"
#include <string>

using namespace std ;
using namespace SEM ;

const int surtidores_gasolina = 3,
          surtidores_gasoil   = 2,
          coches_gasolina     = 6,
          coches_gasoil       = 4;

Semaphore gasolina_libre = surtidores_gasolina,
          gasoil_libre   = surtidores_gasoil;

int surtidores_en_uso = 0;


//**********************************************************************
// plantilla de función para generar un entero aleatorio uniformemente
// distribuido entre dos valores enteros, ambos incluidos
// (ambos tienen que ser dos constantes, conocidas en tiempo de compilación)
//----------------------------------------------------------------------

template< int min, int max > int aleatorio()
{
  static default_random_engine generador( (random_device())() );
  static uniform_int_distribution<int> distribucion_uniforme( min, max ) ;
  return distribucion_uniforme( generador );
}

//---------------------------------------------------------------------------

/**
 * @brief Función que simula la acción de repostar gasolina en un surtidor
 * @param tipo variable tipo string que se utiliza para indicar el 
 * tipo de combustible que está repostando el coche
 * La función Repostar indica por pantalla que se ha comenzado a repostar
 * y a continuación realiza un retardo aleatorio para simular la acción de repostar
 */
void Repostar(string tipo)
{

   // calcular milisegundos aleatorios de duración de la acción de fumar)
   chrono::milliseconds duracion_repostar( aleatorio<20,200>() );

   // informa de que comienza a repostar

    cout << "Entra un coche de " + tipo + " a repostar (" << duracion_repostar.count() << " milisegundos),";
    cout << "Surtidores en uso: " << surtidores_en_uso << endl;
   // espera bloqueada un tiempo igual a ''duracion_repostar' milisegundos
   this_thread::sleep_for( duracion_repostar);

}

//-----------------------------------------------------------------------------

/**
 * @brief Función que simula la acción de un coche de gastar combustible
 * 
 * @param tipo variable tipo string que se utiliza para indicar el 
 * tipo de combustible que está gastando el coche
 *
 * La función Gastar_combustible también se podría entender como el tiempo que tarda 
 * otro coche en llegar a la gasolinera y ponerse a esperar, sin embargo, para que el
 * código quede más legible y cada hebra se entienda como un coche propio, se ha pensado
 * la función como el tiempo que tarda un coche en gastar su combustible y volver a la 
 * cola de la gasolinera (aunque en la realidad esto no tenga sentido ya que un coche tarda
 * mucho más en gastar el combustible)
 */
void Gastar_combustible(string tipo)
{

   // calcular milisegundos aleatorios de duración de la acción de fumar)
   chrono::milliseconds duracion_gastar( aleatorio<20,200>() );

   // espera bloqueada un tiempo igual a ''duracion_gastar' milisegundos
   this_thread::sleep_for( duracion_gastar);

   cout << "Coche de " + tipo + " Comienza la espera de un surtidor,";
   cout << "Surtidores en uso: " << surtidores_en_uso << endl;

}

//---------------------------------------------------------------------------------

/**
 * @brief Función que cada hebra de gasolina debe ejecutar
 * 
 */
void Funcion_hebra_gasolina()
{
    while(true){
        gasolina_libre.sem_wait();
        surtidores_en_uso++;
        Repostar("gasolina");
        surtidores_en_uso--;
        cout << "Termina coche de gasolina de repostar, surtidores ocupados: " << surtidores_en_uso << endl;
        gasolina_libre.sem_signal();
        Gastar_combustible("gasolina");
    }

}

//-----------------------------------------------------------

/**
 * @brief Función que cada hebra de gasoil debe ejecutar
 * 
 */
void Funcion_hebra_gasoil()
{
    while(true){
        gasoil_libre.sem_wait();
        surtidores_en_uso++;
        Repostar("gasoil");
        surtidores_en_uso--;
        cout << "Termina coche de gasoil de repostar, surtidores ocupados: " << surtidores_en_uso << endl;
        gasoil_libre.sem_signal();
        Gastar_combustible("gasoil");
    }

}

//---------------------------------------------------------

int main(){

    cout <<"---------------------------------------------------------" << endl
        << "               Problema de la gasolinera                 " << endl
        << "---------------------------------------------------------" << endl
        << flush ;
    
    thread hebras_gasolina[coches_gasolina] , hebras_gasoil[coches_gasoil];
        for(int i = 0 ; i < coches_gasolina ; i++){
            hebras_gasolina[ i ] = thread(Funcion_hebra_gasolina);
        }

        for(int i = 0 ; i < coches_gasoil ; i++){
            hebras_gasoil[ i ] = thread(Funcion_hebra_gasoil);
        }

        for(int i = 0 ; i < coches_gasolina ; i++){
            hebras_gasolina[ i ].join();
        }

        for(int i = 0 ; i < coches_gasoil ; i++){
            hebras_gasoil[ i ].join();
        }
}