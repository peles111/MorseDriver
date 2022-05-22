#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <semaphore.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MSG_LEN 50
#define MAX_LEN (MSG_LEN + 49) // for #
#define TEST_LEN 5
#define PART_UNIT 1
#define LETTER_UNIT 3
#define SPACE_UNIT 1
#define DEVICE_NAME "/dev/morse"
#define NUM_OF_CHARACTERS 37
#define MAX_DEVICE_NAME 255

static char charset[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 ";

char command_character = ' '; // this char is a shared resource
char *random_array;
int test_ind = -1;

int file_desc;
int ret_val;

pthread_t user_communication;
pthread_t task;

char *test_array[5] = {"MORSE", "TEST", "LINUX EMBEDDED", "2ROOM", "FIFTH ELEMENT"};
char *codes[5] = {"--#---#.-.#...#.", "-#.#...#-", ".-..#..#-.#..-#-..-# #.#--#-...#.#-..#-..#.#-..", "..---#.-.#---#---#--", "..-.#..#..-.#-#....# #.#.-..#.#--#.#-.#-"};
char *output_array;
sem_t shared;
sem_t synchro;

void *user_comm_foo(void *p);
void *task_foo(void *p);

void *user_comm_foo(void *p)
{
    /*commented block that follows is only needed for additional task
     *    char diode_number;
     *    int time_unit;
     *
     *   printf("Enter diode number\n");
     *  scanf("%c", &diode_number);
     *
     *   printf("Enter time_unit\n");
     *  scanf("%d", &time_unit);
     */

    // how to be sure that user inserted one char exactly? Not neccessary but still curious
    while (1)
    {
        while (command_character != 'N' && command_character != 'Q' && command_character != 'T' && command_character != 'n' && command_character != 'q' && command_character != 't')
        {
            if (command_character != '\r' && command_character != '\n')
            {
                printf("\n====================\nEnter work regime\nN/n - normal; T/t - test; !/q - quit\n");
            }
            scanf("%c", &command_character);
            if (command_character == 't' || command_character == 'T')
            {

                do
                {
                    printf("Enter test array index, 0-4\n");
                    scanf("%d", &test_ind);
                } while (test_ind < 0 && test_ind > 4);
            }
        }
        sem_post(&shared);

        if (command_character == 'q' || command_character == 'Q')
        {
            break;
        }
        else
        {
            // waits until task finishes and requests next iteration instead of exiting; maybe
            // way I imagined it isn't quite right
            sem_wait(&synchro);
        }
    }
}

void *task_foo(void *p)
{
    int i;
    const char *dname = (char *)p;

    while (1)
    {
        sem_wait(&shared);

        if (command_character == 'n' || command_character == 'N')
        {
            printf("Normal regime starts!\n");

            printf("%s device is successfully opened! %d\n", dname, file_desc);

            while (1)
            {
                file_desc = open(dname, O_RDWR);

                if (file_desc < 0)
                {
                    printf("%s device isn't open or doesn't exist: exiting application!\n", dname);
                    // printf("Try:\t1) Check does " DEVICE_NAME " node exist\n\t2)'chmod 666 " DEVICE_NAME "\n\t3) "
                    //        "insmod"
                    //        " morse module\n");
                    sem_post(&synchro);
                    exit(-1);
                }

                int len = 0;
                len = rand() % MSG_LEN;
                printf("%d - lenght of the message\n", len);
                random_array = malloc(MSG_LEN * sizeof(char));
                if (random_array)
                {
                    int key;
                    for (i = 0; i < len; i++)
                    {
                        key = rand() % NUM_OF_CHARACTERS;
                        random_array[i] = charset[key];
                    }
                    random_array[len] = '\0';
                    printf("MESSAGE: %s\n", random_array);

                    ret_val = write(file_desc, random_array, len);
                    printf("Written message: %s\n", random_array);
                    printf("write ret_val: %d\n", ret_val);

                    /* Reopen /dev/morse device. */
                    close(file_desc);
                    file_desc = open(dname, O_RDWR);

                    if (file_desc < 0)
                    {
                        printf("%s device isn't open or doesn't exist: exiting application!\n", dname);
                        // printf("Try:\t1) Check does " DEVICE_NAME " node exist\n\t2)'chmod 666 " DEVICE_NAME "\n\t3) "
                        //        "insmod"
                        //        " morse module\n");
                        sem_post(&synchro);
                        exit(-1);
                    }

                    /* Init temp buffer. */
                    memset(output_array, 0, MAX_LEN);

                    /* Read from /dev/morse device. */
                    printf("Reading...\n");
                    ret_val = read(file_desc, output_array, MAX_LEN);
                    printf("Driver has returned message: %s\n", output_array);
                    printf("ret_val: %d\n", ret_val);

                    /* Close /dev/morse device. */
                    close(file_desc);
                }
                free(random_array);
                sleep(2);
            }
            command_character = ' ';
            /*actual job*/
            sem_post(&synchro);
        }
        else if (command_character == 't' || command_character == 'T')
        {
            printf("Test regime starts!\n");
            file_desc = open(dname, O_RDWR);

            if (file_desc < 0)
            {
                printf("%s device isn't open or doesn't exist: exiting application!\n", dname);
                // printf("Try:\t1) Check does " DEVICE_NAME " node exist\n\t2)'chmod 666 " DEVICE_NAME "\n\t3) "
                //        "insmod"
                //        " morse module\n");
                sem_post(&synchro);
                exit(-1);
            }

            printf("%s device is successfully opened! %d\n", dname,  file_desc);

            /* Write to /dev/morse device. */
            ret_val = write(file_desc, test_array[test_ind], strlen(test_array[test_ind]));
            printf("Written message: %s\n", test_array[test_ind]);
            printf("write ret_val: %d\n", ret_val);

            /* Reopen /dev/morse device. */
            close(file_desc);
            file_desc = open(dname, O_RDWR);

            if (file_desc < 0)
            {
                printf("%s device isn't open or doesn't exist: exiting application!\n", dname);
                // printf("Try:\t1) Check does " DEVICE_NAME " node exist\n\t2)'chmod 666 " DEVICE_NAME "\n\t3) "
                //        "insmod"
                //        " morse module\n");
                sem_post(&synchro);
                exit(-1);
            }

            /* Init temp buffer. */
            memset(output_array, 0, MAX_LEN);

            /* Read from /dev/morse device. */
            printf("Reading...\n");
            ret_val = read(file_desc, output_array, MAX_LEN);
            printf("Driver has returned message: %s\n", output_array);
            printf("ret_val: %d\n", ret_val);

            /* Close /dev/morse device. */
            close(file_desc);

            if (!strncmp(output_array, codes[test_ind], strlen(codes[test_ind])))
            {
                printf("Test successful. Morse code validated.\n");
            }
            else
            {
                printf("Test number %d failed.\n", test_ind);
                // sem_post(&synchro);
                // exit(-1);
            }

            command_character = ' ';

            sem_post(&synchro);
        }
        else if (command_character == 'q' || command_character == 'Q')
        {
            printf("Exiting application!\n");
            break;
        }
    }
}

int main(int argc, char *argv[])
{

    if (argc != 2)
    {
        printf("Invalid number of argumens detected! Exiting.\n");
        exit(-1);
    }

    char *dev_name = malloc(MAX_DEVICE_NAME * sizeof(char));
    output_array = malloc(MAX_LEN * sizeof(char));

    srand(time(NULL));
    strncpy(dev_name, argv[1], strlen(argv[1]));

    /*task will block on shared if user_comm hasn't written anything into command_char, after
    user_comm writes it signals semaphore */
    sem_init(&shared, 0, 0);
    sem_init(&synchro, 0, 0);

    pthread_create(&user_communication, NULL, user_comm_foo, (void *)dev_name);
    pthread_create(&task, NULL, task_foo, (void *)dev_name);

    pthread_join(user_communication, NULL);
    pthread_join(task, NULL);

    sem_destroy(&synchro);
    sem_destroy(&shared);

    free(dev_name);

    return 0;
}
