#include <stdio.h>
#include "compression.h"

int main(void){
    int choice1, choice2;
    printf("Welcome to the Image Compression Tool\n");
    printf("This tool is used to compress the image using different algorithms.\n");
    printf("\nWhat is your image type??\n1.PGM image.\n2.BMP image.\n3.JPEG image.\n");
    printf("Enter your choice in number: ");
    scanf("%d", &choice1);
    printf("\n");
    if(choice1 == 1){
        printf("Which algorithm you want to use??\n1.Huffman coding.\n2.Run Length Encoding.\n3.LZW.\n");
        printf("Enter your choice in number: ");
        scanf("%d", &choice2);
        printf("\n");
        if (choice2 == 1)
        {
            huffman();
            
        }
        else if(choice2 == 2){
            rle();
        }
        else if(choice2 == 3)
        {
            lzw();
        }
        else{
            printf("Invalid choice.\n");
        }    

    }
    else if(choice1 == 2){
        printf("Which algorithm you want to use??\n1.Huffman coding.\n2.Run Length Encoding.\n3.LZW.\n");
        printf("Enter your choice in number: ");
        scanf("%d", &choice2);
        printf("\n");
        if (choice2 == 1)
        {
            huffmanBMP();
        } 
        else if(choice2 == 2){
            runlengthBmp();
        }
        else if(choice2 == 3)
        {
            lzwBMP();
        }
        else{
            printf("Invalid choice.\n");
        }

    }
    else if(choice1 == 3){
        printf("For JPEG image, we are using DCT algorithm.\n");
        DCT();  
    }
    else{
        printf("Invalid choice.\n");
    }

    printf("\n");
    printf("Thank you for using the Image Compression Tool");
    printf("\n");
    return 0;
} 
