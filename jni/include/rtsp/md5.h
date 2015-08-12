#ifndef MD5_FORENCRPTY_H
#define MD5_FORENCRPTY_H
/*this is only 32bit*/
#include <stdint.h>
typedef uint32_t md5_int;
struct MD5_struct
{
    md5_int A;
    md5_int B;
    md5_int C;
    md5_int D;
    md5_int lenbuf;    
    char buffer[128];
};
void md5_init(struct MD5_struct *ctx,char * buffer);
void md5_process(struct MD5_struct * ctx);
void md5_fini(struct MD5_struct *ctx,void *rebuf);
void md5_buffer_full(struct MD5_struct * ctx);
void md5_print(struct MD5_struct * ctx);
void MD5_encode(char* message,char buf[33]);
#endif


