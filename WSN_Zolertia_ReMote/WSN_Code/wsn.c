/**
 * \file
 *          Code of the WSN part of the sistem 
 *          "Sistema IoT flexible para el monitoreo 
 *          de variables ambientales en aplicaciones 
 *          agroindustriales"
 * \authors
 *          Geuseppe Segrera <se.geuseppe@javeriana.edu.co>
 *          Javier Galvis    <javieragalvis@javeriana.edu.co.se>
 *          Daniel Castro    <castroda@javeriana.edu.co.se>
 */

/*---------------------------------------------------------------------------*/
#include "contiki.h"
#include "sys/etimer.h"
#include "dev/leds.h"
#include "rtcc.h"
#include "dev/uart.h"
#include "dev/serial-line.h"
#include <stdio.h>
#include <stdint.h>
#include "dev/adc-zoul.h"
#include "dev/zoul-sensors.h"

#include "wsn_funtions.h"
/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define NEG_INF -999

#define REMOTE 1//Habilitar el uso del transmisor
#define MY_TX_POWER_DBM 7//Potencia de TX

#define BEACONS_AMOUNT 4 //Cantidad de beacons que enviaoara cofigurar
#define COUNTDOWN_BEACON 15 //Tiempo en segndos que se espera para recibir beacons
#define COUNTDOWN_NARY 15 //Tiempo en segndos que se espera para recibir Narys

#define TEST_ALARM_MATCH_MIN    0
#define TEST_ALARM_SECOND       30

#define TRIES_MSG 5

/*******************************************************************************
 * Variables
 ******************************************************************************/
struct beacon b;
struct node n;
struct unicast_msg_arry mensaje2;
struct unicast_msg_arry reSendNary;
struct unicast_msg_arry mensajeRetry;

struct preferred_parent *p; // Para recorrer la lista de posibles padres
struct list_unicast_msg *l;
struct list_unicast_msg *rxList;

static uint8_t rtc_buffer[sizeof(simple_td_map)];
static simple_td_map *simple_td = (simple_td_map *)rtc_buffer;


int flag=1;

int waitTimeBeacons=COUNTDOWN_BEACON;

int counterBeacons=0;
int counterNaries=0;
int counterWaitBeacons=0;
int waitForNary=0;
int DelayForBeacon=0;

int ts=10;
int ts0=10;
int ts1=10;
int ts2=10;
int ts3=10;
int ts4=10;
int ts5=10;
int ts6=10;
int ts7=10;
int ts8=10;
int ts9=10;
int able_measure=0;
int receptCheck=0;//Confirmacion de recepcion de mensaje
int waiting_receptCheck=0;//Esperar por confirmacion de recepcion de mensaje
int counterWait_unicastCheck=0;

int NaryRecived=0;

int beacons_checks=0;
int nary_checks=0;
int set_countdown_beacon=0;
int able_to_configure=1;//Variable para habilitar la recepcion de beacons
int able_reset=0;

int rx_or_msg=0;//1=rx,2=msg

MEMB(node_NT_mem, struct node_NT, 30);

MEMB(preferred_parent_mem, struct preferred_parent, 30); // LISTA ENLAZADA
LIST(preferred_parent_list);

MEMB(unicast_msg_mem, struct list_unicast_msg, 30);//LISTA ENLAZADA
LIST(unicast_msg_list);

char Nary_Mensaje[MSG_LENGHT];
char Nary_Recived[MSG_LENGHT];
char Msg_WSN[MSG_LENGHT];

/*---------------------------------------------------------------------------*/
static struct etimer et0;
static struct etimer et1;
static struct etimer et2;
static struct etimer et3;
static struct etimer et4;
static struct etimer et5;
static struct etimer et6;
static struct etimer et7;
static struct etimer et8;
static struct etimer et9;
/*---------------------------------------------------------------------------*/

PROCESS(uart_recv, "cc2538 uart demo");
PROCESS(send_beacon, "Enviar beacons");
PROCESS(select_parent, "Seleccionar padre");
PROCESS(send_measures, "Descubrir vecinos");
PROCESS(send_unicast_NT, "Enviar Nary");
PROCESS(retx_unicast_msg, "Retrasmitir mensaje unicast");
PROCESS(update_routing_table, "Actualizar lista de ruteo");
PROCESS(send_nodes_config, "Enviar configuración desde el nodo raiz");
PROCESS(set_config, "Establecer configuracion enviada");
PROCESS(muestreo_puerto1, "Tiempo muestreo puerto 1");
PROCESS(muestreo_puerto2, "Tiempo muestreo puerto 2");
PROCESS(muestreo_puerto3, "Tiempo muestreo puerto 3");
PROCESS(muestreo_puerto4, "Tiempo muestreo puerto 4");
PROCESS(muestreo_puerto5, "Tiempo muestreo puerto 5");
PROCESS(muestreo_puerto6, "Tiempo muestreo puerto 6");
PROCESS(muestreo_puerto7, "Tiempo muestreo puerto 7");
PROCESS(muestreo_puerto8, "Tiempo muestreo puerto 8");
PROCESS(muestreo_puerto9, "Tiempo muestreo puerto 9");
PROCESS(muestreo_puerto10, "Tiempo muestreo puerto 10");
PROCESS(send_upstream_downstream,"Enviar mesaje en la WSN");
PROCESS(send_unicast_check,"Enviar mensaje de recepcion de unicast");
PROCESS(wating_for_unicast_check,"Esperar a recibir confirmacion y reintentar envio de mensaje");
PROCESS(re_send,"reenviar mensaje si no recibio confirmacion");
PROCESS(countdown_Beacon,"cuenta atras para recibir mas beacons");
PROCESS(countdown_Nary,"cuenta atras para recibir mas Narys");

AUTOSTART_PROCESSES(
  &uart_recv,
  &send_beacon,
  &select_parent,
  &send_measures,
  &send_unicast_NT,
  &retx_unicast_msg,
  &update_routing_table,
  &send_nodes_config,
  &set_config,
  &muestreo_puerto1,
  &muestreo_puerto2,
  &muestreo_puerto3,
  &muestreo_puerto4,
  &muestreo_puerto5,
  &muestreo_puerto6,
  &muestreo_puerto7,
  &muestreo_puerto8,
  &muestreo_puerto9,
  &muestreo_puerto10,
  &send_upstream_downstream,
  &send_unicast_check,
  &wating_for_unicast_check,
  &re_send,
  &countdown_Beacon,
  &countdown_Nary
);
/*---------------------------------------------------------------------------*/
static void broadcast_recv(struct broadcast_conn *c, const linkaddr_t *from)
{
  printf("Recibio_beacon\n");

  if(able_to_configure){//START//Si esta disponible para configurarse la parte simple del arbol

    void *msg = packetbuf_dataptr(); // msg que llego

    struct beacon b_recv = *((struct beacon *)msg);
    // RSSI
    int16_t last_rssi = packetbuf_attr(PACKETBUF_ATTR_RSSI);
    int16_t total_rssi = b_recv.rssi_p + last_rssi;

    struct preferred_parent *in_l; // in to list

    //reiniciar red
    if(able_reset){
      printf("Reset WSN\n");
      counterBeacons=0;
      able_measure=0;
      nary_checks=0;
      beacons_checks=0;
      waitForNary=0;
      able_reset=0;
      reset_nodes(node_arr , 30);

      n.preferred_parent.u8[0]=0;
    }


    if (linkaddr_node_addr.u8[0] != 1) // Si no es el nodo raiz
    {
      printf("RSSI recived from NODE ID %d = '%d'. TOTAL RSSI=%d\n", b_recv.id.u8[0], b_recv.rssi_p, total_rssi);

      // LISTA
      // Revisar si ya conozco este posible padre
      for (p = list_head(preferred_parent_list); p != NULL; p = list_item_next(p))
      {
        // We break out of the loop if the address of the neighbor matches
        // the address of the neighbor from which we received this
        // broadcast message.
        if (linkaddr_cmp(&p->id, &b_recv.id))//Sî ya estaba en la lista de posibles padres
        {
          // YA estaba en la lista ACTUALIZAR
          p->rssi_a = b_recv.rssi_p + last_rssi; // Guardo del rssi. El rssi es igual al rssi_path + rssi del broadcast
          // printf("beacon updated to list with RSSI_A = %d\n", p->rssi_a);
          break;
        }
      }

      // Si no conocia este posible padre
      if (p == NULL)
      {
        // ADD to the listcall
        in_l = memb_alloc(&preferred_parent_mem);
        if (in_l == NULL)
        { // If we could not allocate a new entry, we give up.
          printf("ERROR: we could not allocate a new entry for <<preferred_parent_list>> in tree_rssi\n");
        }
        else
        {
          // Guardo los campos del mensaje
          in_l->id = b_recv.id; // Guardo el id del nodo
          // rssi_ac es el rssi del padre + el rssi del enlace al padre
          in_l->rssi_a = b_recv.rssi_p + last_rssi; // Guardo del rssi acumulado. El rssi acumulado es el rssi divulgado por el nodo (rssi_path) + el rssi medido del beacon que acaba de llegar (rss)
          list_push(preferred_parent_list, in_l);  // Add an item to the start of the list.
          printf("beacon added to list: id = %d rssi_a = %d\n", in_l->id.u8[0], in_l->rssi_a);
        }
      }

    }

    beacons_checks=0;//Reiniciar coenta atras para recibir beacons
    set_countdown_beacon=1;
    process_post(&countdown_Beacon, PROCESS_EVENT_CONTINUE, NULL);
  }//END//Si esta disponible para configurarse la parte simple del arbol

}
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
static void unicast_recv(struct unicast_conn *c, const linkaddr_t *from)//RED CONFIG
{
  void *msg = packetbuf_dataptr(); // msg que llego
  struct unicast_msg_arry u_recv=*((struct unicast_msg_arry *)msg);

  printf("unicast received message=%s, from id=%d, through id=%d \n", u_recv.strPtr, u_recv.origin_id.u8[0], u_recv.from_id.u8[0]);//Mensaje recibido

  //leds_toggle(LEDS_YELLOW);

  l = memb_alloc(&unicast_msg_mem);
  //l = list_head(unicast_msg_list);

  if (l != NULL)
  {
    printf("Adding unicast to the list\n");
    list_push(unicast_msg_list, l);
    //llenar_mensaje(&l,u_recv.from_id,u_recv.strPtr);

    memcpy(&l->strPtr, &u_recv.strPtr, sizeof(l->strPtr));

    //l->strPtr=u_recv.strPtr;
    linkaddr_copy(&l->from_id, &u_recv.from_id);// Set message id
    linkaddr_copy(&l->origin_id, &u_recv.origin_id);// Set message id

    for (l = list_head(unicast_msg_list); l != NULL; l = list_item_next(l))
    {
      printf("Msgs on line:%s, from:%d, through:%d \n",l->strPtr,l->origin_id.u8[0],l->from_id.u8[0]);
    }

  }

  if(u_recv.strPtr[0]=='N' && u_recv.strPtr[1]=='T'){//Recibir un Nary
    printf("Se recibio Nary\n");
    nary_checks=0;

    memcpy(&Nary_Recived[0],&u_recv.strPtr,sizeof(u_recv.strPtr));

    if(waitForNary){
      NaryRecived=1;
      process_post(&update_routing_table, PROCESS_EVENT_CONTINUE, NULL);
      //process_post(&send_unicast_check, PROCESS_EVENT_CONTINUE, NULL);

    }else{
      printf("No esta habilitado para actualizar el Nary!\n");
      l = list_head(unicast_msg_list);

      if(l != NULL){
        list_remove(unicast_msg_list, l);
        memb_free(&unicast_msg_mem, l);
      }
    }


  }else if(u_recv.strPtr[0]=='R' && u_recv.strPtr[1]=='E' && u_recv.strPtr[2]=='C' && u_recv.strPtr[3]=='V'){//Recibir mensaje de confirmacion

    printf("Success sending unicast\n");
    waiting_receptCheck=0;
    counterWait_unicastCheck=0;

    l = list_head(unicast_msg_list);

    if(l != NULL){
      list_remove(unicast_msg_list, l);
      memb_free(&unicast_msg_mem, l);
    }

    for (l = list_head(unicast_msg_list); l != NULL; l = list_item_next(rxList))
    {
      printf("Remaining waiting for check msg2.1:%s\n",l->strPtr);
    }

    //Mensajes que faltan por enviar
    if(rx_or_msg==1){
      /*
      rxList = list_head(uartRX_msg_list);
      if(rxList != NULL){
        list_remove(uartRX_msg_list, rxList);
        memb_free(&uartRX_msg_mem, rxList);
      }

      for (rxList = list_head(uartRX_msg_list); rxList != NULL; rxList = list_item_next(rxList))
      {
        printf("Remaining waiting for check msg1:%s\n",rxList->strPtr);
      }
      */
    }else if(rx_or_msg==2){
      /*
      l = list_head(unicast_msg_list);
      for (l = list_head(unicast_msg_list); l != NULL; l = list_item_next(rxList))
      {
        printf("Remaining waiting for check msg2.1:%s\n",l->strPtr);
      }
      if(l != NULL){
        list_remove(unicast_msg_list, l);
        memb_free(&unicast_msg_mem, l);
      }
      for (l = list_head(unicast_msg_list); l != NULL; l = list_item_next(rxList))
      {
        printf("Remaining waiting for check msg2.2:%s\n",l->strPtr);
      }
      */
    }


  }else{

      NaryRecived=0;

      process_post(&retx_unicast_msg, PROCESS_EVENT_CONTINUE, NULL);

      //process_post(&send_unicast_check, PROCESS_EVENT_CONTINUE, NULL);

    //}
  }
}
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
void
rtcc_interrupt_callback(uint8_t value)
{
  printf("A RTCC interrupt just happened! time/date: ");
  rtcc_print(RTCC_PRINT_DATE_DEC);
  //leds_toggle(LEDS_ALL);
}
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
unsigned int
uart1_send_bytes(const unsigned char *s, unsigned int len)
{
  unsigned int i = 0;

  while(s && *s != 0) {
    if(i >= len) {
      break;
    }
    uart_write_byte(1, *s++);
    i++;
  }
  return i;
}
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
static const struct broadcast_callbacks broadcast_callbacks = {broadcast_recv};
static struct broadcast_conn broadcast;

static const struct unicast_callbacks unicast_callbacks = {unicast_recv};
static struct unicast_conn unicast;
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(send_unicast_check, ev, data)
{

  char check_recep[5];

  //linkaddr_t forwarder_id;

  PROCESS_EXITHANDLER(unicast_close(&unicast);)

  PROCESS_BEGIN();

  unicast_open(&unicast, 146, &unicast_callbacks);

    while (1)
    {
      PROCESS_YIELD();
      if(ev != serial_line_event_message) {

        sprintf(check_recep, "RECV");


        //Llenar el mensaje con el id del nodo emisor
        linkaddr_copy(&mensaje2.from_id, &linkaddr_node_addr);
        linkaddr_copy(&mensaje2.origin_id, &linkaddr_node_addr);

        //Llenar el mensaje con el mensaje de recepcion
        memcpy(&mensaje2.strPtr,&check_recep[0],sizeof(check_recep));

        packetbuf_copyfrom(&mensaje2, sizeof(struct unicast_msg_arry)); // beacon id
        l = list_head(unicast_msg_list);

        if(NaryRecived==1){
          printf("Mensaje de confirmacion a enviar Nary: %s, destino: %d\n",mensaje2.strPtr,l->from_id.u8[0]);

          unicast_send(&unicast, &l->from_id);//Enviar recepcion
          l = list_tail(unicast_msg_list);
          if(l != NULL){
            list_remove(unicast_msg_list, l);
            memb_free(&unicast_msg_mem, l);
          }
          process_post(&send_unicast_NT, PROCESS_EVENT_CONTINUE, NULL);
          //process_post(&update_routing_table, PROCESS_EVENT_CONTINUE, NULL);
        }else if(NaryRecived==0){
          printf("Mensaje de confirmacion a enviar No Nary1: %s, destino: %d\n",mensaje2.strPtr,l->from_id.u8[0]);
          unicast_send(&unicast, &l->from_id);//Enviar recepcion
          l = list_tail(unicast_msg_list);

          if(l != NULL){
            list_remove(unicast_msg_list, l);
            memb_free(&unicast_msg_mem, l);
          }


          //process_post(&retx_unicast_msg, PROCESS_EVENT_CONTINUE, NULL);
        }else{
          printf("Mensaje de confirmacion a enviar No Nary2: %s, destino: %d\n",mensaje2.strPtr,l->from_id.u8[0]);
          unicast_send(&unicast, &l->from_id);//Enviar recepcion
          process_post(&set_config, PROCESS_EVENT_CONTINUE, NULL);
        }


        //printf("Exit send_unicast_check\n");


      }

    }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
char *rxdata;
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(uart_recv, ev, data)//Recepción mensaje uart
{


  PROCESS_BEGIN();
  uart_set_input(1, serial_line_input_byte);
  //leds_toggle(LEDS_GREEN);

  while(1) {
    PROCESS_YIELD();
    if(ev == serial_line_event_message) {

      //leds_toggle(LEDS_RED);
      rxdata = data;
      printf("Data received over UART %s\n", rxdata);


      if(linkaddr_node_addr.u8[0] == 1){//Si es el nodo raíz

        if( (rxdata[0]=='1') && (rxdata[1]=='/') ){
          sprintf(Msg_WSN, "%d,'%s'\r\n",linkaddr_node_addr.u8[0],serializeV3(node_arr[linkaddr_node_addr.u8[0]-1]));
          uart1_send_bytes((uint8_t *)Msg_WSN, sizeof(Msg_WSN) - 1);
        }else if((rxdata[0]=='C') && (rxdata[1]=='G')){//reconfigurar la red
          counterBeacons=0;
          beacons_checks=0;
          nary_checks=0;
          waitForNary=0;
          able_measure=0;
          able_to_configure=1;
          reset_nodes(node_arr , 30);
          n.preferred_parent.u8[0]=0;
        }else{
          process_post(&send_nodes_config, PROCESS_EVENT_CONTINUE, NULL);
        }

      }else{
        process_post(&send_measures, PROCESS_EVENT_CONTINUE, NULL);
      }
    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
int forwarder;
char config_msg[50];
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(send_nodes_config, ev, data)//Enviar configuración a los nodos
{

  PROCESS_BEGIN();

  while(1) {
    PROCESS_YIELD();
    if(ev != serial_line_event_message) {

      int destino;

      if(rxdata[1] != '/'){//Si el nodo es de dos cifras
        destino = (int_cast(rxdata[0]) *10) + int_cast(rxdata[1]);//Nodo de dos cifras
      }else{
        destino = int_cast(rxdata[0]);//Nodo de una cifra
      }

      printf("destino:%d\n",destino);

      if(able_measure){

        if(rxdata[1] != '/'){//Si el nodo es de dos cifras
          destino = (int_cast(rxdata[0]) *10) + int_cast(rxdata[1]);//Nodo de dos cifras
        }else{
          destino = int_cast(rxdata[0]);//Nodo de una cifra
        }

        if( destino!=linkaddr_node_addr.u8[0] ){//Si el nodo de destino no este nodo

          printf("Origen:%d,Destino:%d\n",node_arr[linkaddr_node_addr.u8[0]-1]->id,destino);

          printf("Lista de ruteo:%s\n",serializeV3(node_arr[linkaddr_node_addr.u8[0]-1]));

          forwarder=search_forwarder(node_arr[linkaddr_node_addr.u8[0]-1],destino);
          printf("flag forwarder\n");

          linkaddr_t forwarder_id;

          if(forwarder==0){
            printf("El nodo %d no existe en la red\n",  destino);
          }else if(forwarder==linkaddr_node_addr.u8[0]){//Si el destino es uno de sus hijos

              //printf("cond1\n");
              forwarder_id.u8[0] = destino;//Segunda cifra del id al que enviar el mensaje
            //forwarder_id.u8[0] = int_cast(l->strPtr[0]);
          }else{
              //printf("cond3\n");
              forwarder_id.u8[0] = forwarder;//Segunda cifra del id al que enviar el mensaje
          }

          forwarder_id.u8[1] = 0;

          printf("Transmisor config:%d%d\n",forwarder_id.u8[1],forwarder_id.u8[0]);

          printf("Forwarder config:%d\n", forwarder);

          //Llenar el mensaje con el id del nodo emisor
          mensaje2.from_id.u8[1]=0;
          mensaje2.from_id.u8[0]=1;

          //config_msg=rxdata;

          //Llenar el mensaje con el Nary
          memcpy(&mensaje2.strPtr,rxdata,sizeof(mensaje2.strPtr));

          packetbuf_copyfrom(&mensaje2, sizeof(struct unicast_msg_arry)); // beacon id
          //packetbuf_copyfrom(&l->id, sizeof(linkaddr_t));
          if(forwarder!=0){//Si el nodo existe en la lista
            printf("sending msg:%s, from:%d, to:%d\n", mensaje2.strPtr,mensaje2.from_id.u8[0],forwarder_id.u8[0]);

            unicast_send(&unicast, &forwarder_id);

            //Guardar mensaje en lista en caso de no recibirse

            memcpy(&reSendNary.strPtr, &mensaje2.strPtr, sizeof(reSendNary.strPtr));

            //l->strPtr=u_recv.strPtr;
            linkaddr_copy(&reSendNary.from_id, &mensaje2.from_id); // Set from who
            linkaddr_copy(&reSendNary.origin_id, &mensaje2.from_id); // Set from who
            linkaddr_copy(&reSendNary.to_id, &forwarder_id); // Set to who

            waiting_receptCheck=1;
            rx_or_msg=1;
          }
      }
      }
    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(select_parent, ev, data)
{
  PROCESS_BEGIN();

  struct preferred_parent *p; // Para recorrer la lista de posibles padres
  int lowest_rssi;
  linkaddr_t new_parent;

  while (1)
  {
    PROCESS_WAIT_EVENT();
    if(ev != serial_line_event_message) {

      //printf("Entering select_parent\n");
      if (linkaddr_node_addr.u8[0] != 1) // Si no es el nodo raiz
      {
        if (list_length(preferred_parent_list) >= 1)
        {
          // Find the lowest of the list
          // Assume that the first one is the lowest
          // recorrer la LISTA
          printf("-------\n");
          for (p = list_head(preferred_parent_list), lowest_rssi = p->rssi_a, linkaddr_copy(&new_parent, &p->id); p != NULL; p = list_item_next(p))
          {
            printf("LISTA ID=%d RSSI_A = %d \n", p->id.u8[0], p->rssi_a);
            if (lowest_rssi < p->rssi_a)
            {
              lowest_rssi = p->rssi_a;
              linkaddr_copy(&new_parent, &p->id);
            }
          }
          printf("-------\n");

          // update_parent
          if (n.preferred_parent.u8[0] != new_parent.u8[0])
          {
            printf("#L %d 0\n", n.preferred_parent.u8[0]); // 0: old parent Quitar linea de simulaciòn hacia el padre anterior
            //printf("lowest_rssi = %d \n", lowest_rssi);

            if(flag){//Iniciar nodos para el nary
              init_nodes(node_arr , 30);
              flag=0;
            }

            linkaddr_copy(&n.preferred_parent, &new_parent);

            node_arr[n.preferred_parent.u8[0]-1]->child=NULL;//Eliminar el hijo del padre anterior
            add_child_v2(node_arr[n.preferred_parent.u8[0]-1],node_arr[linkaddr_node_addr.u8[0]-1]);//Cambiar el padre del nodo en le nary
            printf("node: %d \n",node_arr[6]->id);
            printf("new parent: %d , %d \n",node_arr[n.preferred_parent.u8[0]-1]->id,node_arr[linkaddr_node_addr.u8[0]-1]->id);
            printf("#L %d 1\n", n.preferred_parent.u8[0]); //: 1: new parent //agregar linea de simulaciòn al nuevo padre
          }
          // update parent rssi
          n.rssi_p = lowest_rssi; // total_rssi + lowest_rssi
          // Eliminar la lista de padres preferidos
        }
      }
      //printf("Exit send_unicast_check\n");

    }

  }
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(retx_unicast_msg, ev, data)//Retransmitir mensaje entrante
{
  PROCESS_EXITHANDLER(unicast_close(&unicast);)

  PROCESS_BEGIN();

  unicast_open(&unicast, 146, &unicast_callbacks);

  while (1)
  {
    PROCESS_YIELD();

    if(ev != serial_line_event_message) {

      printf("Entering retx_unicast_msg\n");
      l = list_head(unicast_msg_list);

      if (l!=NULL)
      {
        //l = list_head(unicast_msg_list);
        //llenar_mensaje(&mensaje2,l->from_id,l->strPtr);
        int destino;
        printf("Origen:%d\n",l->from_id.u8[0]);

        if(l->strPtr[1] != '/'){//Si el nodo es de dos cifras
          destino = (int_cast(l->strPtr[0]) *10) + int_cast(l->strPtr[1]);//Nodo de dos cifras
        }else{
          destino = int_cast(l->strPtr[0]);//Nodo de una cifra
        }

        printf("destino:%d\n",destino);

        if(destino!=linkaddr_node_addr.u8[0]){//Si el nodo de destino no este nodo

          printf("Origen:%d,Destino:%d\n",node_arr[linkaddr_node_addr.u8[0]-1]->id,destino);

          printf("Lista de ruteo:%s\n",serializeV3(node_arr[linkaddr_node_addr.u8[0]-1]));

          forwarder=search_forwarder(node_arr[linkaddr_node_addr.u8[0]-1],destino);
          //printf("flag forwarder\n");

          linkaddr_t forwarder_id;

          if(forwarder==0){
            forwarder_id.u8[0]=n.preferred_parent.u8[0];
          }else if(forwarder==linkaddr_node_addr.u8[0]){//Si el destino es uno de sus hijos

              //printf("cond1\n");
              forwarder_id.u8[0] = destino;//Segunda cifra del id al que enviar el mensaje
            //forwarder_id.u8[0] = int_cast(l->strPtr[0]);
          }else{
              //printf("cond3\n");
              forwarder_id.u8[0] = forwarder;//Segunda cifra del id al que enviar el mensaje
          }
          forwarder_id.u8[1] = 0;

          printf("Transmisor:%d%d\n",forwarder_id.u8[1],forwarder_id.u8[0]);

          printf("Forwarder:%d\n", forwarder);

          //Llenar el mensaje con el id del nodo emisor
          linkaddr_copy(&mensaje2.from_id, &linkaddr_node_addr);

          //Llenar el mensaje con el id del nodo emisor
          linkaddr_copy(&mensaje2.origin_id, &l->origin_id);

          //Llenar el mensaje con el Nary
          memcpy(&mensaje2.strPtr,&l->strPtr,sizeof(mensaje2.strPtr));

          packetbuf_copyfrom(&mensaje2, sizeof(struct unicast_msg_arry)); // beacon id
          //packetbuf_copyfrom(&l->id, sizeof(linkaddr_t));
          unicast_send(&unicast, &forwarder_id);

          waiting_receptCheck=1;
          rx_or_msg=2;

          memcpy(&reSendNary.strPtr, &mensaje2.strPtr, sizeof(reSendNary.strPtr));

          //l->strPtr=u_recv.strPtr;
          linkaddr_copy(&reSendNary.from_id, &mensaje2.from_id); // Set from who
          linkaddr_copy(&reSendNary.origin_id, &mensaje2.origin_id); // Set from who
          linkaddr_copy(&reSendNary.to_id, &n.preferred_parent); // Set to who

          process_post(&send_unicast_check, PROCESS_EVENT_CONTINUE, NULL);

          //list_remove(unicast_msg_list, l);
          //memb_free(&unicast_msg_mem, l);

        }else{
          printf("El mensaje: %s ,llego al destino\n",l->strPtr);
          //list_remove(unicast_msg_list, l);
          //memb_free(&unicast_msg_mem, l);
          //printf("El mensaje: %s ,llego al destino\n",l->strPtr);
          NaryRecived=2;
          process_post(&send_unicast_check, PROCESS_EVENT_CONTINUE, NULL);
          //process_post(&set_config, PROCESS_EVENT_CONTINUE, NULL);
        }
      }
      printf("Exit retx_unicast_msg\n");

    }

  }
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(set_config, ev, data)//Accion de los nodos al recibir un mensaje con destino hacia ellos
{
  static uint32_t batt;
  int top = 3300;
  int bottom = 2700;
  int range = top - bottom;
  int rangeVolts;
  char prueba_msg[MSG_LENGHT];
  int percent;
  PROCESS_BEGIN();
  uart_set_input(1, serial_line_input_byte);
  printf("RE-Mote RTC test\n");
  //char measure_date[MSG_LENGHT];
  RTCC_REGISTER_INT1(rtcc_interrupt_callback);

  while (1)
  {
    PROCESS_YIELD();

    if(ev != serial_line_event_message) {
      remove_element(l->strPtr,0,MSG_LENGHT);
      remove_element(l->strPtr,0,MSG_LENGHT);
      if(l->strPtr[0]=='/'){
        remove_element(l->strPtr,0,MSG_LENGHT);
      }
      printf("Configuracion Recibida: %s \n",l->strPtr);
      if(l->strPtr[0]=='C' && l->strPtr[1]=='T'){//Configurar fecha RTC
        set_date(simple_td,l->strPtr);
        if( rtcc_set_time_date(simple_td) == AB08_ERROR){
          printf("No se pudo establecer la hora en el RTC\n");
        }
        rtcc_get_time_date(simple_td);
        get_date(simple_td);

        //rtcc_get_date(RTCC_PRINT_DATE_DEC,measure_date);
        //printf("seted date:%s\n",measure_date);
      }else if(l->strPtr[0]=='C' && l->strPtr[1]=='P'){//Configuracion de los puertos

        remove_element(l->strPtr,0,MSG_LENGHT);
        remove_element(l->strPtr,0,MSG_LENGHT);
        remove_element(l->strPtr,0,MSG_LENGHT);
        sprintf(prueba_msg, "%s", l->strPtr);
        printf("Writing UART:%s\n",prueba_msg);

        able_measure=0;
        uart_write_byte(1, 'C');
        uart1_send_bytes((uint8_t *)prueba_msg, sizeof(prueba_msg) - 1);
        able_measure=1;

      }else if(l->strPtr[0]=='M'){//Recibio medida imprimir en el serial nodo sumidero

        remove_element(l->strPtr,0,MSG_LENGHT);
        printf("Writing UART:%s\n",l->strPtr);
        able_measure=0;
        uart1_send_bytes((uint8_t *)l->strPtr, sizeof(l->strPtr) - 1);
        able_measure=1;

      } else if(l->strPtr[0]=='G' && l->strPtr[1]=='M'){//Obtener medida actual

        remove_element(l->strPtr,0,MSG_LENGHT);
        remove_element(l->strPtr,0,MSG_LENGHT);
        printf("Getting measures\n");
        if(linkaddr_node_addr.u8[0]!=1){
          uart_write_byte(1, 'R');
        }

      }else if(l->strPtr[0]=='C' && l->strPtr[1]=='M'){//Configuracion tiempo de muestreo

        //all in one msg
        remove_element(l->strPtr,0,MSG_LENGHT);
        remove_element(l->strPtr,0,MSG_LENGHT);
        remove_element(l->strPtr,0,MSG_LENGHT);
        able_measure=0;
        if(l->strPtr[1]!=':'){
          ts0=(int_cast(l->strPtr[0])*100)+(int_cast(l->strPtr[1])*10)+(int_cast(l->strPtr[2]));
          PROCESS_CONTEXT_BEGIN(&muestreo_puerto1);
          etimer_set(&et0, CLOCK_SECOND * ts0 );
          PROCESS_CONTEXT_END(&muestreo_puerto1);
          ts1=(int_cast(l->strPtr[4])*100)+(int_cast(l->strPtr[5])*10)+(int_cast(l->strPtr[6]));
          PROCESS_CONTEXT_BEGIN(&muestreo_puerto2);
          etimer_set(&et1, CLOCK_SECOND * ts1 );
          PROCESS_CONTEXT_END(&muestreo_puerto2);
          ts2=(int_cast(l->strPtr[8])*100)+(int_cast(l->strPtr[9])*10)+(int_cast(l->strPtr[10]));
          PROCESS_CONTEXT_BEGIN(&muestreo_puerto3);
          etimer_set(&et2, CLOCK_SECOND * ts2 );
          PROCESS_CONTEXT_END(&muestreo_puerto3);
          ts3=(int_cast(l->strPtr[12])*100)+(int_cast(l->strPtr[13])*10)+(int_cast(l->strPtr[14]));
          PROCESS_CONTEXT_BEGIN(&muestreo_puerto4);
          etimer_set(&et3, CLOCK_SECOND * ts3 );
          PROCESS_CONTEXT_END(&muestreo_puerto4);
          ts4=(int_cast(l->strPtr[16])*100)+(int_cast(l->strPtr[17])*10)+(int_cast(l->strPtr[18]));
          PROCESS_CONTEXT_BEGIN(&muestreo_puerto5);
          etimer_set(&et4, CLOCK_SECOND * ts4 );
          PROCESS_CONTEXT_END(&muestreo_puerto5);
          ts5=(int_cast(l->strPtr[20])*100)+(int_cast(l->strPtr[21])*10)+(int_cast(l->strPtr[22]));
          PROCESS_CONTEXT_BEGIN(&muestreo_puerto6);
          etimer_set(&et5, CLOCK_SECOND * ts5 );
          PROCESS_CONTEXT_END(&muestreo_puerto6);
          ts6=(int_cast(l->strPtr[24])*100)+(int_cast(l->strPtr[25])*10)+(int_cast(l->strPtr[26]));
          PROCESS_CONTEXT_BEGIN(&muestreo_puerto7);
          etimer_set(&et6, CLOCK_SECOND * ts6 );
          PROCESS_CONTEXT_END(&muestreo_puerto7);
          ts7=(int_cast(l->strPtr[28])*100)+(int_cast(l->strPtr[29])*10)+(int_cast(l->strPtr[30]));
          PROCESS_CONTEXT_BEGIN(&muestreo_puerto8);
          etimer_set(&et7, CLOCK_SECOND * ts7 );
          PROCESS_CONTEXT_END(&muestreo_puerto8);
          ts8=(int_cast(l->strPtr[32])*100)+(int_cast(l->strPtr[33])*10)+(int_cast(l->strPtr[34]));
          PROCESS_CONTEXT_BEGIN(&muestreo_puerto9);
          etimer_set(&et8, CLOCK_SECOND * ts8 );
          PROCESS_CONTEXT_END(&muestreo_puerto9);
          ts9=(int_cast(l->strPtr[36])*100)+(int_cast(l->strPtr[37])*10)+(int_cast(l->strPtr[38]));
          PROCESS_CONTEXT_BEGIN(&muestreo_puerto10);
          etimer_set(&et9, CLOCK_SECOND * ts9 );
          PROCESS_CONTEXT_END(&muestreo_puerto10);
        }else{
          if(l->strPtr[0]=='0'){
            ts0=(int_cast(l->strPtr[2])*100)+(int_cast(l->strPtr[3])*10)+(int_cast(l->strPtr[4]));
            PROCESS_CONTEXT_BEGIN(&muestreo_puerto1);
            etimer_set(&et0, CLOCK_SECOND * ts0 );
            PROCESS_CONTEXT_END(&muestreo_puerto1);
          }else if(l->strPtr[0]=='1'){
            ts1=(int_cast(l->strPtr[2])*100)+(int_cast(l->strPtr[3])*10)+(int_cast(l->strPtr[4]));
            PROCESS_CONTEXT_BEGIN(&muestreo_puerto2);
            etimer_set(&et1, CLOCK_SECOND * ts1 );
            PROCESS_CONTEXT_END(&muestreo_puerto2);
          }else if(l->strPtr[0]=='2'){
            ts2=(int_cast(l->strPtr[2])*100)+(int_cast(l->strPtr[3])*10)+(int_cast(l->strPtr[4]));
            PROCESS_CONTEXT_BEGIN(&muestreo_puerto3);
            etimer_set(&et2, CLOCK_SECOND * ts2 );
            PROCESS_CONTEXT_END(&muestreo_puerto3);
          }else if(l->strPtr[0]=='3'){
            ts3=(int_cast(l->strPtr[2])*100)+(int_cast(l->strPtr[3])*10)+(int_cast(l->strPtr[4]));
            PROCESS_CONTEXT_BEGIN(&muestreo_puerto4);
            etimer_set(&et3, CLOCK_SECOND * ts3 );
            PROCESS_CONTEXT_END(&muestreo_puerto4);
          }else if(l->strPtr[0]=='4'){
            ts4=(int_cast(l->strPtr[2])*100)+(int_cast(l->strPtr[3])*10)+(int_cast(l->strPtr[4]));
            PROCESS_CONTEXT_BEGIN(&muestreo_puerto5);
            etimer_set(&et4, CLOCK_SECOND * ts4 );
            PROCESS_CONTEXT_END(&muestreo_puerto5);
          }else if(l->strPtr[0]=='5'){
            ts5=(int_cast(l->strPtr[2])*100)+(int_cast(l->strPtr[3])*10)+(int_cast(l->strPtr[4]));
            PROCESS_CONTEXT_BEGIN(&muestreo_puerto6);
            etimer_set(&et5, CLOCK_SECOND * ts5 );
            PROCESS_CONTEXT_END(&muestreo_puerto6);
          }else if(l->strPtr[0]=='6'){
            ts6=(int_cast(l->strPtr[2])*100)+(int_cast(l->strPtr[3])*10)+(int_cast(l->strPtr[4]));
            PROCESS_CONTEXT_BEGIN(&muestreo_puerto7);
            etimer_set(&et6, CLOCK_SECOND * ts6 );
            PROCESS_CONTEXT_END(&muestreo_puerto7);
          }else if(l->strPtr[0]=='7'){
            ts7=(int_cast(l->strPtr[2])*100)+(int_cast(l->strPtr[3])*10)+(int_cast(l->strPtr[4]));
            PROCESS_CONTEXT_BEGIN(&muestreo_puerto8);
            etimer_set(&et7, CLOCK_SECOND * ts7 );
            PROCESS_CONTEXT_END(&muestreo_puerto8);
          }else if(l->strPtr[0]=='8'){
            ts8=(int_cast(l->strPtr[2])*100)+(int_cast(l->strPtr[3])*10)+(int_cast(l->strPtr[4]));
            PROCESS_CONTEXT_BEGIN(&muestreo_puerto9);
            etimer_set(&et8, CLOCK_SECOND * ts8 );
            PROCESS_CONTEXT_END(&muestreo_puerto9);
          }else if(l->strPtr[0]=='9'){
            ts9=(int_cast(l->strPtr[2])*100)+(int_cast(l->strPtr[3])*10)+(int_cast(l->strPtr[4]));
            PROCESS_CONTEXT_BEGIN(&muestreo_puerto10);
            etimer_set(&et9, CLOCK_SECOND * ts9 );
            PROCESS_CONTEXT_END(&muestreo_puerto10);
          }
        }
        able_measure=1;

      }else if(l->strPtr[0]=='P' && l->strPtr[1]=='B'){//Obtener porcentaje de bateria

        remove_element(l->strPtr,0,MSG_LENGHT);
        remove_element(l->strPtr,0,MSG_LENGHT);
        printf("Getting battery %%\n");
        batt = vdd3_sensor.value(CC2538_SENSORS_VALUE_TYPE_CONVERTED);

        rangeVolts = (int)batt - bottom;
        percent = (rangeVolts * 100 / range) ;

        if(percent > 100){
          percent = 100;
        }
        if(percent < 0) {
          percent = 0;
        }
        printf("VDD = %u mV\n", (uint16_t)batt);
        printf("Porcentaje Bat = %d %%\n", percent);
        //sprintf(Msg_WSN, "1/M[%d,B,%d,%u]\r\n",linkaddr_node_addr.u8[0],percent,(uint16_t)batt);
        sprintf(Msg_WSN, "1/M[%d,'B',%d]\r\n",linkaddr_node_addr.u8[0],percent);
        process_post(&send_upstream_downstream, PROCESS_EVENT_CONTINUE, NULL);

      }else if(l->strPtr[0]=='P' && l->strPtr[1]=='M'){//Obtener tiempo de muestreo

        sprintf(Msg_WSN, "1/M[%d,M,%d]\r\n",linkaddr_node_addr.u8[0],ts);
        process_post(&send_upstream_downstream, PROCESS_EVENT_CONTINUE, NULL);

      }else if(l->strPtr[0]=='P' && l->strPtr[1]=='R'){//Obtener tabla de ruteo nodo

        if(node_arr[linkaddr_node_addr.u8[0]-1]->child != NULL){
          sprintf(Msg_WSN, "1/M%d%s\r\n",linkaddr_node_addr.u8[0],serializeV3(node_arr[linkaddr_node_addr.u8[0]-1]));

        }else{
          sprintf(Msg_WSN, "1/M%d))\r\n",linkaddr_node_addr.u8[0]);
        }
        process_post(&send_upstream_downstream, PROCESS_EVENT_CONTINUE, NULL);

      }

      l = list_head(unicast_msg_list);

      if(l != NULL){
        list_remove(unicast_msg_list, l);
        memb_free(&unicast_msg_mem, l);
      }
    }
  }
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(send_measures, ev, data)//Enviar mensaje de las medidas al padre
{

  PROCESS_EXITHANDLER(unicast_close(&unicast);)

  PROCESS_BEGIN();

  unicast_open(&unicast, 146, &unicast_callbacks);

  char measure_date[25];

  char prueba_msg[MSG_LENGHT];
  char msg_measures[MSG_LENGHT];

    while (1)
    {
      PROCESS_YIELD();
      if(ev != serial_line_event_message) {

        if(able_measure){

          rtcc_get_time_date(simple_td);
          rtcc_get_date(RTCC_PRINT_DATE_DEC,measure_date);
          printf("Measured date:%s\n",measure_date);

          strcpy(prueba_msg, rxdata);//Mensaje downstream prueba
          printf("prueba_msg: %s\n",prueba_msg);
          printf("Measured date:%s\n",measure_date);
          sprintf(msg_measures, "1/M[%d,'%s','%s']\r\n", linkaddr_node_addr.u8[0],measure_date,prueba_msg);
          printf("msg_measures: %s\n",msg_measures);

          //Llenar el mensaje con el id del nodo emisor
          linkaddr_copy(&mensaje2.from_id, &linkaddr_node_addr);
          //Llenar el mensaje con el id del nodo de origen
          linkaddr_copy(&mensaje2.origin_id, &linkaddr_node_addr);
          //Llenar el mensaje con el Nary
          memcpy(&mensaje2.strPtr,&msg_measures[0],sizeof(mensaje2.strPtr));

          //printf("Mensaje a enviar: %p\n",mensaje2.strPtr);
          printf("Mensaje a enviar: %s\n",mensaje2.strPtr);

          packetbuf_copyfrom(&mensaje2, sizeof(struct unicast_msg_arry)); // beacon id

            //printf("ENVIANDO UNICAST\n");
          unicast_send(&unicast, &n.preferred_parent);

      }
    }

  }
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(send_upstream_downstream, ev, data)
{

  PROCESS_EXITHANDLER(unicast_close(&unicast);)

  PROCESS_BEGIN();

  unicast_open(&unicast, 146, &unicast_callbacks);

  int destino;
  linkaddr_t forwarder_id;

    while (1)
    {
      PROCESS_YIELD();
      if(ev != serial_line_event_message) {

        if(able_measure){



          if(Msg_WSN[1] != '/'){//Si el nodo es de dos cifras
            destino = (int_cast(Msg_WSN[0]) *10) + int_cast(Msg_WSN[1]);//Nodo de dos cifras
          }else{
            destino = int_cast(Msg_WSN[0]);//Nodo de una cifra
          }

          if(destino==1){
            forwarder_id.u8[0]=n.preferred_parent.u8[0];
          }else{//Si el destino es uno de sus hijos

              //printf("cond1\n");
             if(search_forwarder(node_arr[linkaddr_node_addr.u8[0]-1],destino)!=0){
                forwarder_id.u8[0] = search_forwarder(node_arr[linkaddr_node_addr.u8[0]-1],destino);
              }else{
                forwarder_id.u8[0]=n.preferred_parent.u8[0];
              }
            //forwarder_id.u8[0] = int_cast(l->strPtr[0]);
          }

          //Llenar el mensaje con el id del nodo emisor
          linkaddr_copy(&mensaje2.from_id, &linkaddr_node_addr);
          //Llenar el mensaje
          memcpy(&mensaje2.strPtr,&Msg_WSN[0],sizeof(mensaje2.strPtr));

          //printf("Mensaje a enviar: %p\n",mensaje2.strPtr);
          printf("Mensaje a enviar: %s\n",mensaje2.strPtr);

          packetbuf_copyfrom(&mensaje2, sizeof(struct unicast_msg_arry)); // beacon id

          unicast_send(&unicast, &forwarder_id);


      }
    }

  }
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(send_unicast_NT, ev, data)
{

  PROCESS_EXITHANDLER(unicast_close(&unicast);)

  PROCESS_BEGIN();

  unicast_open(&unicast, 146, &unicast_callbacks);

    while (1)
    {
      PROCESS_YIELD();
      if(ev != serial_line_event_message) {

        //printf("Entering send_unicast_NT\n");
                if(flag){//inicializar nodos
                  init_nodes(node_arr , 30);
                  flag=0;
                }

                if (linkaddr_node_addr.u8[0] != 1) // Si no es el nodo raiz
                {
                  //Llenar el mensaje con el id del nodo emisor
                  linkaddr_copy(&mensaje2.from_id, &linkaddr_node_addr);
                  linkaddr_copy(&mensaje2.origin_id, &linkaddr_node_addr);

                  printf("Starting serialize\n");

                  //Llenar el mensaje con el Nary
                  memcpy(&mensaje2.strPtr,&serializeV3(node_arr[n.preferred_parent.u8[0]-1])[0],sizeof(mensaje2.strPtr));
                  //Ingresar el mensaje al buffer del unicast
                  packetbuf_copyfrom(&mensaje2, sizeof(struct unicast_msg_arry)); //beacon id

                  printf("serialize msg:%s\n",mensaje2.strPtr);

                  printf("Padre al que se lo va a enviar:%d%d\n",n.preferred_parent.u8[1],n.preferred_parent.u8[0]);//padre
                  //Enviar unicast
                  unicast_send(&unicast, &n.preferred_parent);

                  //Guardar mensaje en lista en caso de no recibirse

                  printf("Guardando mensaje para reintento\n");
                  counterWait_unicastCheck=0;

                  //llenar_mensaje(&l,u_recv.from_id,u_recv.strPtr);

                  memcpy(&reSendNary.strPtr, &mensaje2.strPtr, sizeof(reSendNary.strPtr));

                  //l->strPtr=u_recv.strPtr;
                  linkaddr_copy(&reSendNary.from_id, &mensaje2.from_id); // Set from who
                  linkaddr_copy(&reSendNary.origin_id, &mensaje2.origin_id); // Set from who
                  linkaddr_copy(&reSendNary.to_id, &n.preferred_parent); // Set to who


                  waiting_receptCheck=1;
                  rx_or_msg=1;

                  //Activar cuenta atras para recibir Narys
                  printf("Start Waiting for nary\n");
                  waitForNary=1;
                  able_reset=1;
                  //printf("Exit send_unicast_NT\n");
                  process_post(&countdown_Nary, PROCESS_EVENT_CONTINUE, NULL);

                }


        //printf("Exit send_unicast_NT\n");



      }

    }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(countdown_Beacon, ev, data)//Cuenta atras para dejar de recibir beacons
{
  static struct etimer et;

  PROCESS_BEGIN();

    while (1)
    {

      //printf("Entering countdown_Beacon\n");
            if(set_countdown_beacon){//Activar timer


              etimer_set(&et, CLOCK_SECOND);
              PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

              beacons_checks++;

              if(beacons_checks>=waitTimeBeacons){//Finaliza la cuenta atras para recibir beacons
                printf("No more beacons!\n");
                set_countdown_beacon=0;//Desactivar cuenta atras
                able_to_configure = 0;//Desactivar la recepcion de beacons mientras termina de configurar
                if(linkaddr_node_addr.u8[0]==1){//si es el nodo raiz
                  printf("Start Waiting for nary\n");
                  waitForNary=1;
                  able_reset=1;
                  //printf("Exit countdown_Beacon\n");
                  process_post(&countdown_Nary, PROCESS_EVENT_CONTINUE, NULL);
                }else{
                  //printf("Exit countdown_Beacon\n");
                  process_post(&send_unicast_NT, PROCESS_EVENT_CONTINUE, NULL);//Enviar codificacion del Nary
                }

              }else{
                printf("waiting for beacon %d s left \n",waitTimeBeacons-beacons_checks);
              }
            }else{
              PROCESS_YIELD();
            }
            if(linkaddr_node_addr.u8[0]!=1 && beacons_checks<=1){//Seguir con el proceso de creacion de padre simple
              //printf("Exit countdown_Beacon\n");
              process_post(&select_parent, PROCESS_EVENT_CONTINUE, NULL);
            }
            //printf("Exit countdown_Beacon\n");
    }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(countdown_Nary, ev, data)//Cuenta atras para dejar de recibir Narys
{
  static struct etimer et;

  PROCESS_BEGIN();

    while (1)
    {

      //printf("Entering countdown_Nary\n");
      if(waitForNary){//Activar timer

        etimer_set(&et, CLOCK_SECOND);
        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

        nary_checks++;

        if(nary_checks>=COUNTDOWN_NARY){//Finaliza la cuenta atras para recibir Narys
          waitForNary=0;//Desactivar cuenta atras
          able_to_configure = 1;//Desactivar la recepcion de beacons mientras termina de configurarse
          waiting_receptCheck=0;
          if(linkaddr_node_addr.u8[0]==1){//Si es el nodo raiz
            printf("Tree completed!\n%s\n",serializeV3(node_arr[linkaddr_node_addr.u8[0]-1]));
            //Enviar codificacion del Nary al PC
            sprintf(Msg_WSN, "[%d,'%s']\r\n",linkaddr_node_addr.u8[0],serializeV3(node_arr[linkaddr_node_addr.u8[0]-1]));
            uart1_send_bytes((uint8_t *)Msg_WSN, sizeof(Msg_WSN) - 1);
          }else{
            printf("Tree completed!\n%s\n",serializeV3(node_arr[n.preferred_parent.u8[0]-1]));
          }
          able_measure=1;
        }else{
          printf("waiting for Nary %ds left\n",COUNTDOWN_NARY-nary_checks);
        }
      }else{
        PROCESS_YIELD();
      }
//printf("Exit countdown_Nary\n");

    }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(wating_for_unicast_check, ev, data)//Delay para recibir nuevos beacons y reiniciarse
{
  static struct etimer et;


  PROCESS_BEGIN();

    while (1)
    {

      //printf("Entering wating_for_unicast_check\n");
            etimer_set(&et, CLOCK_SECOND * 5 );

            PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
            if(waiting_receptCheck){
              counterWait_unicastCheck++;
              printf("REsending tries:%d\n", counterWait_unicastCheck);
              process_post(&re_send, PROCESS_EVENT_CONTINUE, NULL);
            }
            if(counterWait_unicastCheck>=TRIES_MSG){
              waiting_receptCheck=0;
              counterWait_unicastCheck=0;

              if(counterBeacons>BEACONS_AMOUNT){
                printf("Deleting Node and reconfiguring network\n");
                counterNaries=0;
                counterBeacons=0;
                counterWaitBeacons=0;
                nary_checks=0;
                //waitForNary=0;
                //DelayForBeacon=0;
                able_measure=0;
                reset_nodes(node_arr , 30);
                n.preferred_parent.u8[0]=0;
              }
            }
      //printf("Exit wating_for_unicast_check\n");

    }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(re_send, ev, data)
{
  PROCESS_EXITHANDLER(unicast_close(&unicast);)

  PROCESS_BEGIN();

  unicast_open(&unicast, 146, &unicast_callbacks);

    while (1)
    {
      PROCESS_YIELD();
      if(ev != serial_line_event_message) {

        //printf("Entering re_send\n");
          //rxList = list_head(uartRX_msg_list);

          printf("REsending msg:%s, from:%d, to:%d\n", reSendNary.strPtr,reSendNary.from_id.u8[0],reSendNary.to_id.u8[0]);

          //Llenar el mensaje con el id del nodo emisor
          linkaddr_copy(&mensajeRetry.from_id, &reSendNary.from_id);
          linkaddr_copy(&mensajeRetry.origin_id, &reSendNary.origin_id);
          //Llenar el mensaje con el Nary
          memcpy(&mensajeRetry.strPtr,&reSendNary.strPtr,sizeof(mensajeRetry.strPtr));

          packetbuf_copyfrom(&mensajeRetry, sizeof(struct unicast_msg_arry)); //beacon id

          unicast_send(&unicast, &reSendNary.to_id);//Enviar recepcion

  //printf("Exit re_send\n");

      }

    }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
PROCESS_THREAD( muestreo_puerto1, ev, data)
{
  PROCESS_BEGIN();
    while (1)
    {
      etimer_set(&et0, CLOCK_SECOND * ts0 );
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et0));

      if(linkaddr_node_addr.u8[0]!=1 && able_measure==1){
        printf("Getting Measures port1\n");
        rtcc_get_time_date(simple_td);
        rtcc_print(RTCC_PRINT_DATE_DEC);
        uart_write_byte(1, '0');
      }

    }
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
PROCESS_THREAD( muestreo_puerto2, ev, data)
{
  PROCESS_BEGIN();
    while (1)
    {
      etimer_set(&et1, CLOCK_SECOND * ts1 );
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et1));

      if(linkaddr_node_addr.u8[0]!=1 && able_measure==1){
        printf("Getting Measures port2\n");
        rtcc_get_time_date(simple_td);
        rtcc_print(RTCC_PRINT_DATE_DEC);
        uart_write_byte(1, '1');
      }

    }
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
PROCESS_THREAD( muestreo_puerto3, ev, data)
{
  PROCESS_BEGIN();
    while (1)
    {
      etimer_set(&et2, CLOCK_SECOND * ts2 );
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et2));

      if(linkaddr_node_addr.u8[0]!=1 && able_measure==1){
        printf("Getting Measures port3\n");
        rtcc_get_time_date(simple_td);
        rtcc_print(RTCC_PRINT_DATE_DEC);
        uart_write_byte(1, '2');
      }

    }
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
PROCESS_THREAD( muestreo_puerto4, ev, data)
{
  PROCESS_BEGIN();
    while (1)
    {
      etimer_set(&et3, CLOCK_SECOND * ts3 );
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et3));

      if(linkaddr_node_addr.u8[0]!=1 && able_measure==1){
        printf("Getting Measures port4\n");
        rtcc_get_time_date(simple_td);
        rtcc_print(RTCC_PRINT_DATE_DEC);
        uart_write_byte(1, '3');
      }

    }
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
PROCESS_THREAD( muestreo_puerto5, ev, data)
{
  PROCESS_BEGIN();
    while (1)
    {
      etimer_set(&et4, CLOCK_SECOND * ts4 );
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et4));

      if(linkaddr_node_addr.u8[0]!=1 && able_measure==1){
        printf("Getting Measures port5\n");
        rtcc_get_time_date(simple_td);
        rtcc_print(RTCC_PRINT_DATE_DEC);
        uart_write_byte(1, '4');
      }

    }
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
PROCESS_THREAD( muestreo_puerto6, ev, data)
{
  PROCESS_BEGIN();
    while (1)
    {
      etimer_set(&et5, CLOCK_SECOND * ts5 );
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et5));

      if(linkaddr_node_addr.u8[0]!=1 && able_measure==1){
        printf("Getting Measures port6\n");
        rtcc_get_time_date(simple_td);
        rtcc_print(RTCC_PRINT_DATE_DEC);
        uart_write_byte(1, '5');
      }

    }
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
PROCESS_THREAD( muestreo_puerto7, ev, data)
{
  PROCESS_BEGIN();
    while (1)
    {
      etimer_set(&et6, CLOCK_SECOND * ts6 );
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et6));

      if(linkaddr_node_addr.u8[0]!=1 && able_measure==1){
        printf("Getting Measures port7\n");
        rtcc_get_time_date(simple_td);
        rtcc_print(RTCC_PRINT_DATE_DEC);
        uart_write_byte(1, '6');
      }

    }
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
PROCESS_THREAD( muestreo_puerto8, ev, data)
{
  PROCESS_BEGIN();
    while (1)
    {
      etimer_set(&et7, CLOCK_SECOND * ts7 );
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et7));

      if(linkaddr_node_addr.u8[0]!=1 && able_measure==1){
        printf("Getting Measures port8\n");
        rtcc_get_time_date(simple_td);
        rtcc_print(RTCC_PRINT_DATE_DEC);
        uart_write_byte(1, '7');
      }

    }
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
PROCESS_THREAD( muestreo_puerto9, ev, data)
{
  PROCESS_BEGIN();
    while (1)
    {
      etimer_set(&et8, CLOCK_SECOND * ts8 );
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et8));

      if(linkaddr_node_addr.u8[0]!=1 && able_measure==1){
        printf("Getting Measures port9\n");
        rtcc_get_time_date(simple_td);
        rtcc_print(RTCC_PRINT_DATE_DEC);
        uart_write_byte(1, '8');
      }

    }
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
PROCESS_THREAD( muestreo_puerto10, ev, data)
{
  PROCESS_BEGIN();
    while (1)
    {
      etimer_set(&et9, CLOCK_SECOND * ts9 );
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et9));

      if(linkaddr_node_addr.u8[0]!=1 && able_measure==1){
        printf("Getting Measures port10\n");
        rtcc_get_time_date(simple_td);
        rtcc_print(RTCC_PRINT_DATE_DEC);
        uart_write_byte(1, '9');
      }

    }
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(update_routing_table, ev, data)
{
  PROCESS_EXITHANDLER(unicast_close(&unicast);)

  PROCESS_BEGIN();

  unicast_open(&unicast, 146, &unicast_callbacks);

  while (1)
  {
    PROCESS_YIELD();
    if(ev != serial_line_event_message) {

      //printf("Entering update_routing_table\n");
            if(flag){
              init_nodes(node_arr , 30);
              flag=0;
            }

            printf("Funcion update Nary=%s\n", Nary_Recived);

            deserialize(Nary_Recived);//Actualizar el Nary

            printf("updated nary:%s\n",serializeV3(node_arr[linkaddr_node_addr.u8[0]-1]));

            //Enviar la codificacion
            process_post(&send_unicast_check, PROCESS_EVENT_CONTINUE, NULL);
            //process_post(&send_unicast_NT, PROCESS_EVENT_CONTINUE, NULL);
      //printf("Exit update_routing_table\n");
    }


  }
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(send_beacon, ev, data)//RED CONFIG
{
  static struct etimer et;

  PROCESS_EXITHANDLER( broadcast_close(&broadcast); )

  PROCESS_BEGIN();

  #if REMOTE
    static radio_value_t val;
    if(NETSTACK_RADIO.set_value(RADIO_PARAM_TXPOWER, MY_TX_POWER_DBM) == RADIO_RESULT_OK)
    {
      NETSTACK_RADIO.get_value(RADIO_PARAM_TXPOWER, &val);
      printf("Trasmission Power Set: %d dBm\n", val);
    }
    else if(NETSTACK_RADIO.set_value(RADIO_PARAM_TXPOWER, MY_TX_POWER_DBM) == RADIO_RESULT_INVALID_VALUE)
    {
      printf("ERROR: RADIO_RESULT_INVALID_VALUE\n");
    }else
    {
      printf("ERROR: The TX power could not be set\n");
    }
  #endif

  broadcast_open(&broadcast, 129, &broadcast_callbacks);

  if (linkaddr_node_addr.u8[0] == 1)// Si es el nodo raiz
  {
    n.rssi_p = 0;
    counterBeacons=BEACONS_AMOUNT+1; //No comenzar enviando beacons
    able_reset=1;
    waitForNary=0;//Desactivar cuenta atras
    able_to_configure = 1;//Desactivar la recepcion de beacons mientras termina de configurarse
    waiting_receptCheck=0;
    nary_checks=COUNTDOWN_NARY+1;

    waitTimeBeacons=COUNTDOWN_BEACON-1;
  }
  else
  {
    n.rssi_p = NEG_INF;
  }

  while (1)
  {
    //printf("Entering send_beacon\n");
      /* Delay 2-4 seconds */
      etimer_set(&et, (CLOCK_SECOND * 4 + random_rand() % (CLOCK_SECOND * 4)) - 300); // 1 3

      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

    if(counterBeacons<=BEACONS_AMOUNT){

      printf("counterBeaconsSended:%d\n",counterBeacons);

      llenar_beacon(&b, linkaddr_node_addr, n.rssi_p);

      packetbuf_copyfrom(&b, sizeof(struct beacon));
      broadcast_send(&broadcast);
      counterBeacons++;

    }
//printf("Exit send_beacon\n");
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
