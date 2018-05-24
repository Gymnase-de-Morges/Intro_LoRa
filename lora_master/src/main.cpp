/*
Lab server to test LoRa functionalities (see courses notes for more details)
Feb 2018
Philippe.Rochat'at´gymorges.ch
Based on work of: Ramin Sangesari

The program is using a modified version of RadioHead (by Ramin Sangesari)
That does support dragino GPS/LoRa hat.
*/

/*-----------------------------------------*/
#include <dirent.h>
#include <string.h>
#include <fcntl.h>
/*-----------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <RH_RF95.h>
#include <queue>


using namespace std; // This makes STL library available
// STL is more specifically used for queue in msg mailboxes

RH_RF95 rf95;

/* The address of the node which is 0 by default */
uint8_t node_number = 0;
int run = 1;

uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];

#define MSG_HELLO_NET 0
#define MSG_ADDR_NET  1
#define MSG_START_T   2
#define MSG_CHALLENGE 3
#define MSG_SYS_ON    4
#define MSG_SYS_OFF   5

char HMsg[] = "Hello World !"; // Welcoming message
uint8_t chkeys[255]; // Last challenge key for max 255 clients

class NodeMsg {
  static int count;
  int nb=0;
  uint8_t *data;
  int len;
  uint8_t from;
  uint8_t to;

public:
  NodeMsg() : NodeMsg(NULL, 0) { };

  NodeMsg(uint8_t *data, int len) : NodeMsg(0,0, data, len) { };

  NodeMsg(uint8_t to, uint8_t from, uint8_t *data, int len) {
    this->to = to;
    this->from = from;
    this->len = len+2;
    this->data = (uint8_t *)malloc(len+2);
    this->data[0] = to;
    this->data[1] = from;
    memcpy(this->data+2, data, len);
    this->nb = count++;

  }
  
  virtual ~NodeMsg() {
    free(this->data);
  }

  uint8_t *getData() {
    return this->data;
  }

  int getLen() {
    return this->len;
  }

  int getNb() {
    return this->nb;
  }
};

int NodeMsg::count = 0;

queue<NodeMsg*> OutBox;

/* Send all message in the OutBox every 3 seconds */
void sigalarm_handler(int signal) {
    NodeMsg *c;

    while(!OutBox.empty()) {
      c = OutBox.front();
      rf95.send((const uint8_t *)c->getData(), c->getLen());
      rf95.waitPacketSent();
      printf("Sent msg number[%d] to[%d] from[%d]: %s\n", c->getNb(), c->getData()[0], c->getData()[1], c->getData()+2);
      delete(c);
      OutBox.pop();
    }

    // refill OutBox with one first hello world
    c = new NodeMsg((uint8_t *)&HMsg[0], strlen(HMsg));
    OutBox.push(c);
    
    alarm(3);
}

void getMsg(uint8_t *msg, int len) {
  switch(msg[2]) {
  case MSG_HELLO_NET:
    printf("HELLO addr query for: %x:%x:%x:%x:%x:%x\n", msg[3], msg[4], msg[5], msg[6], msg[7], msg[8]);
    break;
  case MSG_START_T:
    uint8_t reply[3];
    reply[0] = MSG_CHALLENGE;
    reply[1] = rand() % 255;
    reply[2] = rand() % 255;
    chkeys[msg[1]] = reply[1] + reply[2];
    NodeMsg *c;
    c = new NodeMsg(msg[1], msg[0], reply, 3);
    OutBox.push(c);
    printf("This is a start transaction msg, reply is: [%d] with CHK(%d+%d)\n", c->getNb(), reply[1], reply[2]);
    break;
  case MSG_SYS_ON:
    printf("SYS ON command: ");
    if(chkeys[msg[1]] == msg[3]) {
      printf("ACCEPTED\n");
    } else {
      printf("REJECTED\n");
    }
    break;
  case MSG_SYS_OFF:
    printf("SYS OFF command: ");
    if(chkeys[msg[1]] == msg[3]) {
      printf("ACCEPTED\n");
    } else {
      printf("REJECTED\n");
    }
    break;
  default:
    printf("Err! Unknown msg type\n");
  }
}

/* Signal the end of the software */
void sigint_handler(int signal) {
    run = 0;
}

void setup()
{
    wiringPiSetupGpio();

    if (!rf95.init()) 
    {
        fprintf(stderr, ">Init failed\n");
        exit(1);
    }

    /* Tx power is from +5 to +23 dBm */
    rf95.setTxPower(23);
    /* There are different configurations
     * you can find in lib/radiohead/RH_RF95.h */
    rf95.setModemConfig(RH_RF95::Bw125Cr45Sf128);
    rf95.setFrequency(868.0); /* Mhz */
}

void loop()
{

  /* If we receive one message we show on the prompt
   * the address of the sender and the Rx power.
   */
  if( rf95.available() ) {
    uint8_t len = sizeof(buf);

    if (rf95.recv(buf, &len)) {
      printf(">> Received: %d octets from %d to %d\n",len, buf[1], buf[0]);
      fflush(stdout);
      getMsg(buf, len);
    }
  }

  sleep(100);
}

int main(int argc, char **argv)
{
    if( argc == 2 )
        node_number = atoi(argv[1]);
	
    signal(SIGINT, sigint_handler);
    signal(SIGALRM, sigalarm_handler);
    alarm(3);
    
    setup();
    while( run ) {
        loop();
        usleep(1);
    }
    
    return EXIT_SUCCESS;
}
