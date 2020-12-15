#include <iostream>
#include <cassert>
#include <thread>
#include <mutex>
#include <random> // dispositivos, generadores y distribuciones aleatorias
#include <chrono> // duraciones (duration), unidades de tiempo
#include "HoareMonitor.h"

using namespace std;
using namespace HM;


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


class Bebercio : public HoareMonitor  //Monitor que gestiona el servicio del bar
{
    private:
    CondVar
        barra_libre,        //Variable para esperar a que la barra esté libre
        servido_gin,        //Variable para esperar que preparen gin-tonic
        servido_mojito,     //Variable para esperar que preparen mojito
        servido_daiquiri,   //Variable para esperar que preparen daiquiri
        servido_vermu;      //Variable para esperar que preparen vermu
    int 
        cocktail_servido;   //Entero para no servir siempre el mismo cocktail
    bool 
        barra_ocupada,      //Variable booleana para indicar que la barra está ocupada
        servidogin,
        servidomoj,
        servidodai,
        servidover;            //Variable booleana para indicar que el cocktail está servido
    public:
        Bebercio();
        int elegir_cocktail();
        void servir_cocktail(int indice);
        void pedir_cocktail(int indice);


};

Bebercio::Bebercio(){           //Constructor
    barra_libre = newCondVar();
    servido_gin = newCondVar();
    servido_mojito = newCondVar();
    servido_daiquiri = newCondVar();
    servido_vermu = newCondVar();

    cocktail_servido = -1;
    barra_ocupada = false;
    servidogin = false;
        servidomoj = false;
    servidodai = false;
    servidover = false;

}

int Bebercio::elegir_cocktail(){            //Método para que el barman decida que cocktail hacer...
    int cocktail_elegido = aleatorio<0,3>();
    while(cocktail_elegido == cocktail_servido)
        cocktail_elegido = aleatorio<0,3>();

    cocktail_servido = cocktail_elegido;

    return cocktail_elegido;
}

void Bebercio::servir_cocktail(int indice){ //Método para servir el cocktail...
    if(barra_ocupada)
        barra_libre.wait();
    
    barra_ocupada = true;
    servido = true;

    if(indice == 0)
        servido_gin.signal();
    else if(indice == 1)
        servido_mojito.signal();
    else if(indice == 2)
        servido_daiquiri.signal();
    else if(indice == 3)
        servido_vermu.signal();
}

void Bebercio::pedir_cocktail(int indice){  //Método para que los amigos pidan su cocktail...
    if(indice == 0 && !servidogin)
        servido_gin.wait();
    else if(indice == 1 && !servidomoj)
        servido_mojito.wait();
    else if(indice == 2 && !servidodai)
        servido_daiquiri.wait();
    else if(indice == 3 && !servidover)
        servido_vermu.wait();

    if(barra_ocupada){
        barra_libre.signal();
        barra_ocupada = false;  
    }
    
}

void hebra_barman(MRef<Bebercio> monitor, int indice){ //Hebra que ejecutan los camareros
    while(true){
        int cocktail = monitor->elegir_cocktail();
        chrono::milliseconds duracion_preparar( aleatorio<20,200>() );
        cout<<"El barman "<<indice<<" esta preparando el cocktail "<<cocktail<<"..."<<endl;
        this_thread::sleep_for( duracion_preparar );
        monitor->servir_cocktail(cocktail);
    }
}

void hebra_bebedor(MRef<Bebercio> monitor, int indice){ //Hebra que ejecutan los bebedores
    while(true){
        monitor->pedir_cocktail(indice);
        cout<<"El bebedor "<<indice<<" ha recibido su cocktail y comienza a beber tranquilamente..."<<endl;
        chrono::milliseconds duracion_beber( aleatorio<20,200>() );
        this_thread::sleep_for( duracion_beber );
        cout<<"El bebedor "<<indice<<" ha terminado su cocktail y comienza a esperar..."<<endl;
    }
}


int main(){

    MRef<Bebercio> monitor = Create<Bebercio>();

    thread bebedores[4];
    thread camareros[2];

    for(int i = 0 ; i < 4 ; i++){
        bebedores[i] = thread(hebra_bebedor , monitor, i);
    }

    for(int i = 0 ; i < 2 ; i++){
        camareros[i] = thread(hebra_barman,monitor,i);
    }

    for(int i = 0 ; i < 4 ; i++){
        bebedores[i].join();
    }

    for(int i = 0 ; i < 2 ; i++){
        camareros[i].join();
    }
}

