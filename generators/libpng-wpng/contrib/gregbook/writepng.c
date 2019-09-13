#include <stdlib.h>     /* for exit() prototype */
#include <zlib.h>
#include <sys/random.h>

#include "png.h"        /* libpng header, includes setjmp.h */
#include "writepng.h"   /* typedefs, common macros, public prototypes */
#include "incbin.h"

INCBIN(iccp_embedded, "SMPTE-C.icc");

static png_const_charp exif_bytes = "\x4d\x4d\x0\x2a\x0\x0\x0\x8\x0\x2\x1\x12\x0\x3\x0\x0\x0\x1\x0\x5\x0\x0\x87\x69\x0\x4\x0\x0\x0\x1\x0\x0\x0\x26\x0\x0\x0\x0\x0\x1\xa4\x5\x0\x3\x0\x0\x0\x1\x1\x2c\x0\x0";
/* local prototype */

static void writepng_error_handler(png_structp png_ptr, png_const_charp msg);

int writepng_init(mainprog_info *mainprog_ptr)
{
    png_structp  png_ptr;       /* note:  temporary variables! */
    png_infop  info_ptr;
    png_infop  info_ptr2;
    int color_type, interlace_type;

    //alarm(2);

    /* la sporca: initialize random gen, we pick a rand val every time we have
     * to choose a specific feature, so we can continue to easily explore the space
     * of generable samples is this better than pit files?
     * maybe, if pit files cannot include an on the fly mod to grammar,
     * we might hit earlier "interesting" conditions */

    int seed = 0;
    getrandom(&seed, sizeof(seed), GRND_NONBLOCK);
    srand(seed);

    png_ptr = png_create_write_struct(
        png_get_libpng_ver(NULL), mainprog_ptr,
        writepng_error_handler, NULL);

    info_ptr = png_create_info_struct(png_ptr);
    info_ptr2 = png_create_info_struct(png_ptr);

    mainprog_ptr->png_ptr = png_ptr;
    mainprog_ptr->info_ptr = info_ptr;

    mainprog_ptr->image_data = (uch *)(4096+malloc(1024*1024*4)); // keep a "lax" page :)
    mainprog_ptr->row_pointers = (uch **)(4096+malloc(1024*1024*4));

    getrandom(mainprog_ptr->image_data, ranrange(0, 1000), GRND_NONBLOCK);
    //getrandom((4096*4+mainprog_ptr->image_data), ranrange(0, 250), GRND_NONBLOCK);

    png_set_keep_unknown_chunks(png_ptr, 3, NULL, 0);

    png_init_io(png_ptr, mainprog_ptr->outfile);

    setjmp(mainprog_ptr->jmpbuf);
    png_permit_mng_features(png_ptr, PNG_ALL_MNG_FEATURES);
    png_set_compression_level(png_ptr, ranrange(1,9));
    png_set_compression_mem_level(png_ptr, ranrange(1,MAX_MEM_LEVEL));
    png_set_compression_window_bits(png_ptr, ranrange(8,15));
    png_set_compression_method(png_ptr, 8);
    
    int filters[] = {
        PNG_FILTER_NONE, PNG_FILTER_SUB, PNG_FILTER_UP,
        PNG_FILTER_AVG, PNG_FILTER_PAETH, PNG_INTRAPIXEL_DIFFERENCING, PNG_ALL_FILTERS};

    interlace_type = ranrange(PNG_INTERLACE_NONE, PNG_INTERLACE_ADAM7);

    mainprog_ptr->width = ranrange(10,32);
    mainprog_ptr->height = ranrange(10,32);


    int bit_depth = 1 << ranrange(0,4);
    int choice = ranrange(0,4);
    switch (choice)
    {
        case 0:
            color_type = PNG_COLOR_TYPE_GRAY;
            break;
        case 1:
            color_type = PNG_COLOR_TYPE_RGB;
            break;
        case 2:
            color_type = PNG_COLOR_TYPE_PALETTE;
            break;
        case 3:
            color_type = PNG_COLOR_TYPE_GRAY_ALPHA;
            break;
        default:
            color_type = PNG_COLOR_TYPE_RGB_ALPHA;
    }

    png_set_IHDR(png_ptr, info_ptr, 
        mainprog_ptr->height, mainprog_ptr->width,
        bit_depth, color_type, interlace_type,
        PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_BASE);

    setjmp(mainprog_ptr->jmpbuf);
    if (ranrange(0,1))
        png_set_bKGD(png_ptr,info_ptr, mainprog_ptr->image_data);

    if (bit_depth == 16) {
        if (ranrange(0,1))
        png_set_swap(png_ptr);
    }

    if (bit_depth < 8) {
        if (ranrange(0,1))
        png_set_packswap(png_ptr);
    }

    setjmp(mainprog_ptr->jmpbuf);
    if (color_type==PNG_COLOR_TYPE_PALETTE){

        if (ranrange(0,1)){
            png_sPLT_tp plt = malloc(sizeof(png_sPLT_t)*4000);
            plt[0].name = "gfz";
            plt[0].depth = 1 << ranrange(2,5);
            plt[0].nentries = ranrange(1,256);
            plt[0].entries = (png_sPLT_entryp)mainprog_ptr->image_data;
    
            png_set_sPLT(png_ptr, info_ptr, plt, 1);
        }

        png_set_PLTE(png_ptr, info_ptr, (png_color*)mainprog_ptr->image_data, 1<<bit_depth);

        if (ranrange(0,1))
        png_set_hIST(png_ptr, info_ptr, (mainprog_ptr->image_data+(4096*10)));
    }

    if  ((color_type || PNG_COLOR_MASK_ALPHA))
        if (ranrange(0,1))
        png_set_tRNS(
            png_ptr, info_ptr, mainprog_ptr->image_data,
            ranrange(0,16), mainprog_ptr->image_data);
    
    if (color_type ||PNG_COLOR_MASK_COLOR)
    if (ranrange(0,1))
        png_set_sRGB_gAMA_and_cHRM(png_ptr, info_ptr,
            ranrange(PNG_sRGB_INTENT_PERCEPTUAL, PNG_sRGB_INTENT_ABSOLUTE));
    else
    if (ranrange(0,1))
    png_set_iCCP(png_ptr, info_ptr, "gfz", PNG_COMPRESSION_TYPE_BASE,
        giccp_embeddedData, giccp_embeddedSize);
            

    if (color_type == PNG_COLOR_TYPE_RGB || color_type == PNG_COLOR_TYPE_RGBA)
    if (ranrange(0,1))
        png_set_bgr(png_ptr);
    
    //pCAL
    png_charpp params = (mainprog_ptr->image_data+4096);
    int num_params = ranrange(1, 3);
    char fp[] = "0.8e1";
    for (int i=0; i<num_params+5; i++){
        params[i] = fp;
    }

    if (ranrange(0,1))
    png_set_pCAL(
    png_ptr,info_ptr, "Display", 2, 1,
        1, num_params,
        "METERS", params);

    //invert
    if (ranrange(0,1))
    if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
        png_set_invert_mono(png_ptr);

    if (ranrange(0,1))
    if (color_type == PNG_COLOR_TYPE_GA || color_type == PNG_COLOR_TYPE_RGBA)
       png_set_invert_alpha(png_ptr);

    if (ranrange(0,30)==0)
    png_set_filter(png_ptr, 0, filters[ranrange(0,5)]);



    if (ranrange(0,1)){
        png_time  modtime;
        png_convert_from_time_t(&modtime, mainprog_ptr->modtime);
        png_set_tIME(png_ptr, info_ptr, &modtime);
    }

    if (ranrange(0,1))
    png_set_eXIf_1(png_ptr, info_ptr, 53, exif_bytes);

    if (ranrange(0,1))
    png_set_oFFs(
        png_ptr, info_ptr, rand(), rand(),
        ranrange(PNG_OFFSET_PIXEL, PNG_OFFSET_MICROMETER));
    
    if (ranrange(0,1))
    png_set_pHYs(png_ptr, info_ptr, rand(), rand(), ranrange(PNG_RESOLUTION_UNKNOWN, PNG_RESOLUTION_METER));

    if (ranrange(0,1))
    png_set_sCAL(png_ptr, info_ptr, ranrange(PNG_SCALE_METER, PNG_SCALE_LAST-1), rand()%256, rand()%256);

    // sBIT
    png_color_8 p8;
    p8.alpha = ranrange(1,8);
    p8.red = ranrange(1,8);
    p8.blue = ranrange(1,8);
    p8.green = ranrange(1,8);
    p8.gray = ranrange(1,8);
    if (ranrange(0,1000)==0)
    png_set_sBIT(png_ptr, info_ptr, &p8);

    png_write_info(png_ptr, info_ptr);


    int num_text = 1;// ranrange(0,6);
    png_textp text = malloc(4096*sizeof(png_textp));
    char *keys[]={"Title", "Author", "Description", "Copyright", "E-mail", "URL", "GFZ", "\x00"};
    for (int i=0; i<num_text+7; i++){
        text[i].compression = ranrange(0, PNG_TEXT_COMPRESSION_LAST)-1; //;ranrange(0, 3)-1;
        text[i].key = keys[ranrange(0,5)];
        text[i].text = "gfz";
        if (ranrange(0,1)){
            text[i].lang = "IT";
            text[i].lang_key = "FR"; // colpo di testa
        }else{
            text[i].lang = NULL;
            text[i].lang_key = NULL;
        }
        // PNG_ITXT/TEXT_COMPRESSION_NONE/zTXt
    }

    // libpng only has one zstream, if idat is used conc
    png_set_text(png_ptr, info_ptr2, text, num_text);

    png_write_info(png_ptr, info_ptr2);

    if (ranrange(0,50))
        png_set_packing(png_ptr);
    //png_set_shift(png_ptr, &sig_bit);  to scale low-bit-depth values 

    int rows = png_get_rows(png_ptr, info_ptr);
    // didn't really figure out why it wants more rows than we expect sometimes \o/
    for (int i = 0;  i < rows+1000; ++i){
        mainprog_ptr->row_pointers[i] = mainprog_ptr->image_data;
        if (interlace_type){
                png_write_row(png_ptr, mainprog_ptr->row_pointers);

            if (ranrange(0,1000)==0)
                png_set_filter(png_ptr, 0, filters[ranrange(0,5)]);

        }
    }

    if (!interlace_type){
        png_write_image(png_ptr, mainprog_ptr->row_pointers);
    }

    png_write_end(png_ptr, NULL);
    return 0;
}


static void writepng_error_handler(png_structp png_ptr, png_const_charp msg)
{
    mainprog_info  *mainprog_ptr;

    fputs(msg,stderr);
    fputc('\n',stderr);

    mainprog_ptr = png_get_error_ptr(png_ptr);

    if (mainprog_ptr && ++mainprog_ptr->errors<5){
        longjmp(mainprog_ptr->jmpbuf, 1);
    }

    fputs("ADDIO :(\n", stderr);
    exit(0xd00d);
}