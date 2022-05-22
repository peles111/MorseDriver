#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <asm/uaccess.h>
#include <asm/string.h>

#define MSG_LEN 50
#define MAX_MSG_LEN (MSG_LEN + 49)
#define MAX_CODE_LEN 6 // zbog terminalnog karaktera
#define NUM_OF_CHARACTERS 37

//as far as I understand, 0 has the longest code of 19 units

static int major_number;
static int slen = 0;

static char message_buffer[MAX_MSG_LEN];
static char code_buffer[MAX_MSG_LEN][MAX_CODE_LEN];

static char* map[NUM_OF_CHARACTERS] = 
{
	".-",
	"-...",
	"-.-.",
	"-..",
	".",
	"..-.",
	"--.",
	"....",
	"..",
	".---",
	"-.-",
	".-..",
	"--",
	"-.",
	"---", //O
	".--.",
	"--.-",
	".-.",
	"...",
	"-",
	"..-",
	"...-",
	".--",
	"-..-",
	"-.--",
	"--..",//Z
	" ",//space
	"-----", //numbers
	".----",
	"..---",
	"...--",
	"....-",
	".....",
	"-....",
	"--...",
	"---..",
	"----."
};

static int __init morse_init(void);
static void __exit  morse_exit(void);
int morse_read(struct file *fildesc, char *buf, size_t len, loff_t *f_pos);
int morse_write(struct file *filedesc, const char *buf, size_t len, loff_t *f_pos);

struct file_operations fops =
{
	owner: THIS_MODULE,
	read: morse_read,
	write: morse_write,
};

int morse_read(struct file *fildesc, char *buf, size_t len, loff_t *f_pos)
{
	int data_size = 0;
	int retval = 0;
	// iz user aplikacije zvati funkciju read za svaki karakter sa len = 5 
    	if (*f_pos == 0) // ako se krece iz pocetka 
    	{
    		int i;
        	/* Get size of valid data. */
        	for(i = 0; i < slen; i++){
        		data_size = strlen(code_buffer[i]);
        		/* Send data to user space. */
        		if (copy_to_user(buf, code_buffer[i], data_size) != 0)
        		{
            			return -EFAULT;
        		}
        		else
        		{
            			//(*f_pos) += data_size;
            			buf+=data_size;
            			retval+=data_size; 
            			if(i != slen -1){
		    			if(copy_to_user(buf,"#",1) != 0) //# character will serve as separtor
		    			{
		    				return -EFAULT;
		    			}
		    			else
		    			{
		    				buf+=1;
						retval += 1;		
		    			}

				}
			}
		}
            	return retval;	
    	}
    	else
    	{
        	return 0;
    	}
  	
  	memset(code_buffer, 0, sizeof(code_buffer));
  	slen = 0;
}

int morse_write(struct file *filedesc, const char *buf, size_t len, loff_t *f_pos)
{
	    int i = 0;

	    /* Reset memory. */
	    memset(message_buffer, 0, MAX_MSG_LEN);

	    /* Get data from user space.*/
	    if (copy_from_user(message_buffer, buf, len) != 0)
	    {
		return -EFAULT;
	    }
	    else
	    {
	    	printk(KERN_INFO "\nMessage buffer in DRIVER content: %s", message_buffer);
		    /* Take code from map*/      	
		for(i=0; i < len; ++i){
			if(message_buffer[i] > 64 && message_buffer[i] < 91){
			
				strncpy(code_buffer[i], map[message_buffer[i] - 'A'], MAX_CODE_LEN);
		  	  	//code_buffer[i] = map[message_buffer[i] - 'A'];	//i really hope this is right
			}
			else if(message_buffer[i] > 47 && message_buffer[i] < 58) 
			{
				strncpy(code_buffer[i], map[message_buffer[i] - 21], MAX_CODE_LEN);
			}
			else if(message_buffer[i] == 32)
			{
				strncpy(code_buffer[i], " ", MAX_CODE_LEN);
			}
			else
			{
				printk(KERN_INFO "\nInvalid char\n");
			}

		}
		slen = i;
		return len;
	    }
	    memset(message_buffer, 0, MAX_MSG_LEN);
}


static int __init morse_init(void)
{
	int retcode;
	int i;
	printk(KERN_INFO "\nInitializing Morse code module.\n");
	retcode = register_chrdev(0, "morse", &fops);
	if(retcode < 0)
	{
		printk(KERN_INFO "Error init driver\n");
		return retcode;
	}
	
	major_number=retcode;
	
	for(i=0; i<MAX_MSG_LEN; i++)
	{
	
		memset(code_buffer[i], 0, MAX_CODE_LEN);

	}

	printk(KERN_INFO "Registered Morse module with maj. number %d\n", major_number);


	return 0;
}

static void __exit  morse_exit(void)
{

	printk(KERN_INFO "Removing Morse code module.\n");
	unregister_chrdev(major_number, "morse");
}


module_init(morse_init);
module_exit(morse_exit);

MODULE_LICENSE("GPL");
