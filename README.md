# sliding-window
Data link layer simulation with sliding-window protocol.

Boboc Madalina 323CD 

----------------------------------------------------------------------------------------------------------------------------------------------------------

Used structures:

	typedef struct {
	    int seq_number;
	    int checksum;
	    char data[MSGSIZE - 2 * sizeof(int)];
	} pkt_t;

	typedef struct {
  		int len;
  		char payload[MSGSIZE];
	} msg;

* everytime a msg is sent, a pkt is packed with de data then copied into the payload field of the msg structure. At the destination, the payload field is casted back to a pkt and the data can be accessed.

----------------------------------------------------------------------------------------------------------------------------------------------------------

Used functions:
	
Description:
	[send.c]:
		Firstly, we compute the bandwidth-delay product then the window size based on arguments $SPEED and $DELAY. 
		The first msg sent contains the file name and the number of packets that the receiver will receive. It's sent using the start-stop protocol. The message is resend until the recv sends akn.

