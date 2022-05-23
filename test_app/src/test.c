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
#define MAX_LEN (MSG_LEN + 49) // for # which help us on reading side
#define TEST_LEN 5
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

pthread_mutex_t mut;

char *test_array[5] = {"MORSE", "TEST", "LINUX EMBEDDED", "2ROOM", "FIFTH ELEMENT"};
char *codes[5] = {"--#---#.-.#...#.", "-#.#...#-", ".-..#..#-.#..-#-..-# #.#--#-...#.#-..#-..#.#-..", "..---#.-.#---#---#--", "..-.#..#..-.#-#....# #.#.-..#.#--#.#-.#-"};
char *output_array;
sem_t shared;
sem_t synchro;

void *user_comm_foo(void *p);
void *task_foo(void *p);

void *user_comm_foo(void *p)
{
   
    while (1)
    {
        pthread_mutex_lock(&mut);

        while (command_character != 'N' && command_character != 'Q' && command_character != 'T' && command_character != 'n' && command_character != 'q' && command_character != 't')
        {
            if (command_character != '\r' && command_character != '\n')
            {
                printf("\n====================\nEnter work regime\nN/n - normal; T/t - test; Q/q - quit\n");
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
        pthread_mutex_unlock(&mut);

        if (command_character == 'q' || command_character == 'Q')
        {
            break;
        }
        else
        {
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
        pthread_mutex_lock(&mut);
        
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
            pthread_mutex_unlock(&mut);

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
            }

            command_character = ' ';
            pthread_mutex_unlock(&mut);

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
        printf("Invalid number of arguments detected! Exiting.\n");
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
    pthread_mutex_init(&mut, NULL);

    pthread_create(&user_communication, NULL, user_comm_foo, (void *)dev_name);
    pthread_create(&task, NULL, task_foo, (void *)dev_name);

    pthread_join(user_communication, NULL);
    pthread_join(task, NULL);

    sem_destroy(&synchro);
    sem_destroy(&shared);

    free(dev_name);
    free(output_array);
    return 0;
}
