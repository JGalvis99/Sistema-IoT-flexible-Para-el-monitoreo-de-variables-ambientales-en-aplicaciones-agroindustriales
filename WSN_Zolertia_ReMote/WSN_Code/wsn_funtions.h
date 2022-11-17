
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

#ifndef TREE_LIB_H
#define TREE_LIB_H

#define MSG_LENGHT 80

////////////////////////////
////////  LIBRARIES  ///////
////////////////////////////
#include "contiki.h"
#include "net/rime/rime.h"
#include "cpu.h"
#include "random.h"
#include "dev/button-sensor.h"
#include "dev/leds.h"
#include "lib/memb.h"
#include "sys/etimer.h"
#include "sys/rtimer.h"
#include <stdio.h>
#include "dev/uart.h"
#include "dev/serial-line.h"
#include "net/netstack.h"
////////////////////////////
////////  DEFINE  //////////
////////////////////////////

////////////////////////////
////////  STRUCT  //////////
////////////////////////////

struct beacon
{
  linkaddr_t id;   // Node id
  int16_t rssi_p; // rssi acumulado del padre
  char strPtr[MSG_LENGHT]; //Mensaje
};

struct node
{
  linkaddr_t preferred_parent;   // Node id
  int16_t rssi_p; // rssi acumulado del padre
};


struct preferred_parent
{
  struct preferred_parent *next;
  linkaddr_t id;   // Node id
  int16_t rssi_a; // rssi acumulado
  //struct ctimer ctimer;
};

struct list_unicast_msg
{
  struct list *next;
  linkaddr_t from_id;//id del emisor
  char strPtr[MSG_LENGHT]; //Mensaje
  linkaddr_t to_id;//id del receptor
  linkaddr_t origin_id;//id del nodo de origen
};

struct unicast_msg
{
    linkaddr_t from_id;//id del emisor
    char *strPtr; //Mensaje
};

struct unicast_msg_arry
{
    linkaddr_t from_id;//id del emisor
    char strPtr[MSG_LENGHT]; //Mensaje
    linkaddr_t to_id;//id del receptor
    linkaddr_t origin_id;//id del nodo de origen
};


typedef struct node_NT node_NT;
struct node_NT {
	int id;
	struct node_NT *parent;
	struct node_NT *sibling;
	struct node_NT *child;
};


node_NT * node_arr[30];

////////////////////////////
////////  DEF STRUCT  //////
////////////////////////////


////////////////////////////
////////  FUNCION //////////
////////////////////////////
void llenar_beacon(struct beacon *b, linkaddr_t id, int16_t rssi_p);

void llenar_mensaje(struct unicast_msg_arry *b, linkaddr_t id, char *msg_send);


void init_nodes(node_NT *n[] , int num_nodes);
void reset_nodes(node_NT *n[], int num_nodes);
node_NT * new_node(int id);
node_NT * add_sibling(node_NT * n, node_NT * new_n) ;
node_NT * add_child(node_NT * n, node_NT * new_n) ;
node_NT *add_child_v2(node_NT *n, node_NT *new_n);
void print_node_decendents(node_NT * n , int level);
void print_tabs(int count);

int search_forwarder(node_NT * n , int id); //int
void search_forwarder_RAW(node_NT *n, int id_dest);
char *serialize(node_NT *n,node_NT *level_node);
void serializeV2_Raw(node_NT *level_node);
void print_routing(char msg[],int index);
char char_cast(int num);
int int_cast(char caract);

char *serializeV2(node_NT *level_node);
char *serializeV3(node_NT *level_node);
void deserialize(char msg[]);


void printNode(node_NT *level_node);
node_NT  *add_parent(node_NT  *n, node_NT  *new_n);
void remove_element(char *array, int index, int array_length);


#endif /* TREE_LIB_H */
