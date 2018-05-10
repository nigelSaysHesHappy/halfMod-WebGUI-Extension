#ifndef nighttpsock
#define nighttpsock

#define VER		"v0.4.9"

class nigSock
{
	public:
		std::string mark;
		std::string ip;
		std::string name;
		std::string cookie;
		std::string body;
		std::string session;
		std::string call;
		std::string req;
		std::string host;
		int port;
		int sock;
		int length;
		int playerFlags;
		bool sendCook;
		bool pastHead;
		bool busy;
		nigSock();
		~nigSock();
		char fin_rsv_opcode;
		
		bool isOpen();
		void assign(int socket, struct sockaddr_in addr);
		int nRead(std::string &buffer,int size);
		int nReadLine(std::string &buffer);
		int nWrite(std::string buffer, int flags = 0);
		int writeFile(std::string filename, int flags = 0);
		void nClose();
		int wsSend(std::string message, int flags = 0);
		void wsReadContent(std::string &buffer, int size, unsigned char fin_rsv_opcode);
};

std::ifstream::pos_type filesize(const char* filename);
std::string txt2amp(std::string in);
std::string dec2hex(unsigned int n);
std::string hexify(std::string in);
std::string decify(std::string in);
std::string wsMessage(std::string message, char fin_rsv_opcode = 129);

#endif

