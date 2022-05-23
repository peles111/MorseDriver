#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <asm/uaccess.h>
#include <asm/string.h>
#include <asm/io.h>
#include <linux/delay.h>
//LED

#define ACT_LED_GPIO_PIN (47)
#define ACT_LED_BLINK_PERIOD_MSEC (1000)
#define ACT_LED_GPIO_PIN_OFFSET (ACT_LED_GPIO_PIN - 40)
#define GPSET1_OFFSET (0x00000020)
#define GPCLR1_OFFSET (0x0000002C)
//LED BLINKING UNITS
#define DOT_UNIT (1)
#define PART_UNIT (1)
#define LETTER_UNIT (3)
#define SPACE_UNIT (4) // LETTER_UNIT + space will give 7
#define DASH_UNIT (3)
#define HASH_UNIT (1)

//ADDR
#define GPIO_BASE_ADDR      (0x3F200000)
#define GPIO_ADDR_SPACE_LEN (0xB4)

#define MSG_LEN 50
#define MAX_MSG_LEN (MSG_LEN + 49)
#define MAX_CODE_LEN 6 // because of terminal char
#define NUM_OF_CHARACTERS 37
#define GPFSEL4_OFFSET 0x00000010

static int major_number;
static int slen = 0;


struct led_dev {
    void __iomem   *regs;               /* Virtual address where the physical GPIO address is mapped */
	u8             gpio;                /* GPIO pin that the LED is connected to */
	u32            blink_period_msec;   /* LED blink period in msec */
	ktime_t        kt;                  /* Blink timer period */
};

static struct led_dev act_led =
{
    regs              : NULL,
    gpio              : ACT_LED_GPIO_PIN,
    blink_period_msec : ACT_LED_BLINK_PERIOD_MSEC,
};

static char message_buffer[MAX_MSG_LEN];
static char code_buffer[MAX_MSG_LEN][MAX_CODE_LEN];

static char* map[NUM_OF_CHARACTERS] = 
{
	".-",		//A
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
	"---", 		
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
	"--..",		//Z
	" ",		//space
	"-----", 	//numbers
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
void SetDiodeAsOutput(void __iomem *regs, u8 pin);
void SetGpioPin(void __iomem *regs, u8 pin);
void ClearGpioPin(void __iomem *regs, u8 pin);

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
    	if (*f_pos == 0) 
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
            			buf+=data_size;
            			retval+=data_size; 
            			if(i != slen -1){
		    			if(copy_to_user(buf,"#",1) != 0) //# character will serve as separtor on reading side
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
	int j = 0;
	int k = 0;

	/* Reset memory. */
	memset(message_buffer, 0, MAX_MSG_LEN);

	/* Get data from user space.*/
	if (copy_from_user(message_buffer, buf, len) != 0)
	{
		return -EFAULT;
	}
	else
	{
		/* Take code from map*/
		for (i = 0; i < len; ++i)
		{
			if (message_buffer[i] > 64 && message_buffer[i] < 91)
			{
				strncpy(code_buffer[i], map[message_buffer[i] - 'A'], MAX_CODE_LEN);
			}
			else if (message_buffer[i] > 47 && message_buffer[i] < 58)
			{
				strncpy(code_buffer[i], map[message_buffer[i] - 21], MAX_CODE_LEN);
			}
			else if (message_buffer[i] == 32)
			{
				strncpy(code_buffer[i], " ", MAX_CODE_LEN);
			}
			else
			{
				printk(KERN_INFO "\nInvalid char\n");
			}
		}

		slen = i;

		for (j = 0; j < slen; j++)
		{
			for (k = 0; k < MAX_CODE_LEN; k++)
			{
				switch (code_buffer[j][k])
				{
				case 0:
					break;

				case '.':
					// signal one unit here
					SetGpioPin(act_led.regs, ACT_LED_GPIO_PIN);
					msleep(DOT_UNIT * ACT_LED_BLINK_PERIOD_MSEC);
					ClearGpioPin(act_led.regs, ACT_LED_GPIO_PIN);
					if (code_buffer[j][k + 1] != 0 && code_buffer[j][k + 1] != '\0')
						msleep(PART_UNIT * ACT_LED_BLINK_PERIOD_MSEC);
					break;

				case '-':
					SetGpioPin(act_led.regs, ACT_LED_GPIO_PIN);
					msleep(DASH_UNIT * ACT_LED_BLINK_PERIOD_MSEC);
					ClearGpioPin(act_led.regs, ACT_LED_GPIO_PIN);
					if (code_buffer[j][k + 1] != 0 && code_buffer[j][k + 1] != '\0')
						msleep(PART_UNIT * ACT_LED_BLINK_PERIOD_MSEC);
					break;

				case ' ':
					ClearGpioPin(act_led.regs, ACT_LED_GPIO_PIN);
					msleep(SPACE_UNIT * ACT_LED_BLINK_PERIOD_MSEC);
					break;
				}

				if (code_buffer[j][k] == 0)
					break;
			}
			msleep(LETTER_UNIT * ACT_LED_BLINK_PERIOD_MSEC);
		}
		memset(message_buffer, 0, MAX_MSG_LEN);
		return len;
	}
	
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

	act_led.regs = ioremap(GPIO_BASE_ADDR, GPIO_ADDR_SPACE_LEN);
    if(!act_led.regs)
    {
        retcode = -ENOMEM;
        unregister_chrdev(major_number, "morse");
		return retcode;
    }

	SetDiodeAsOutput(act_led.regs, ACT_LED_GPIO_PIN);
	SetGpioPin(act_led.regs, ACT_LED_GPIO_PIN);

	printk(KERN_INFO "Registered Morse module with maj. number %d\n", major_number);


	return 0;
}

static void __exit  morse_exit(void)
{

    ClearGpioPin(act_led.regs, ACT_LED_GPIO_PIN);
	printk(KERN_INFO "Removing Morse code module.\n");
	unregister_chrdev(major_number, "morse");
}

void SetDiodeAsOutput(void __iomem *regs, u8 pin)
{
    u32 GPFSELReg_offset;
    u32 tmp;
    u32 mask;

    GPFSELReg_offset = GPFSEL4_OFFSET;
    pin = ACT_LED_GPIO_PIN_OFFSET;

    /* Set gpio pin direction. */
    tmp = ioread32(regs + GPFSELReg_offset);

	// set diode as output: set 1
	mask = 0x1 << (pin * 3);
	tmp |= mask;

	iowrite32(tmp, regs + GPFSELReg_offset);
}

void SetGpioPin(void __iomem *regs, u8 pin)
{
    u32 GPSETreg_offset;
    u32 tmp;

    /* Get base address of gpio set register. */
    GPSETreg_offset = GPSET1_OFFSET;
    pin = pin - 32;

    /* Set gpio. */
    tmp = 0x1 << pin;
    iowrite32(tmp, regs + GPSETreg_offset);
}

void ClearGpioPin(void __iomem *regs, u8 pin)
{
    u32 GPCLRreg_offset;
    u32 tmp;

    /* Get base address of gpio clear register. */
    GPCLRreg_offset = GPCLR1_OFFSET;
    pin = pin - 32;

    /* Clear gpio. */
    tmp = 0x1 << pin;
    iowrite32(tmp, regs + GPCLRreg_offset);
}

module_init(morse_init);
module_exit(morse_exit);

MODULE_LICENSE("GPL");
