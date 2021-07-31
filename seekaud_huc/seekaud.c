/*
 *
 * Seektest2 - attempts 10 seeks for minimum and average delays
 * 
 */

#include "huc.h"

#define TITLE_LINE       1
#define DIRECTION_LINE   3
#define START_LINE       5
#define START_UNDER      6
#define OFFSET_LINE      7
#define OFFSET_UNDER     8
#define TARGET_LINE      9
#define STAT_LINE        11
#define READING_LINE     13
#define MIN_LINE         14
#define AVG_LINE         15

#define ADJUST_DIRECTION 0
#define ADJUST_START     1
#define ADJUST_OFFSET    2
#define RUN_TEST         3

#define DIR_FWD          0
#define DIR_BACK         1

#define INSTRUCT_LINE1  24
#define INSTRUCT_LINE2  25
#define INSTRUCT_LINE3  26

#define PREV_LINE  3
#define CURR_LINE  4
#define NEXT_LINE  10
#define UNDER_LINE 11

#define TIME_LINE  6



#asm

_beepsetup:
    lda #2
    sta psg_ch
    lda #$FF
    sta psg_mainvol
    lda #$90    ; 1.5KHz
    sta psg_freqlo
    stz psg_freqhi
    lda #$0
    sta psg_ctrl
    lda #$1F
    ldx #16
.loop:
    sta psg_wavebuf
    dex
    bne .loop
    lda #$0
    ldx #16
.loop1:
    sta psg_wavebuf
    dex
    bne .loop1
    lda #$FF
    sta psg_pan
    lda #$9f
    sta psg_ctrl
    rts

_beepoff:
    lda #$0
    sta psg_ctrl
    rts

_beepon:
    lda #$9f
    sta psg_ctrl
    rts

_beeplow:
    lda #$90
    sta psg_freqlo
    lda #$9f
    sta psg_ctrl
    rts

_beephigh:
    lda #$46
    sta psg_freqlo
    lda #$9f
    sta psg_ctrl
    rts


; cd_read_sector(void) - references char rec_h, char rec_m, char rec_l, char * target_data
_cd_read_sector:

    lda #2    ; logical block, and play
    sta <_bh

    lda _rec_h       ; disc sector address (LBA address)
    sta <_al
    lda _rec_m
    sta <_ah
    lda _rec_l
    sta <_bl

    call cd_search

    rts

_cd_stop:
    call cd_pause
    rts
 
#endasm 


char rec_h;
char rec_m;
char rec_l;

char target_data[2048];

char pad;
char padhold;
char hex1;
char hex2;

char start_pos[3];
char offset[3];
char target[3];
char result[3];


/* create array of offsets for adjusting startpos/offset: */

const char add_num[] = {
 0x10, 0x00, 0x00,
 0x01, 0x00, 0x00,
 0x00, 0x10, 0x00,
 0x00, 0x01, 0x00,
 0x00, 0x00, 0x10,
 0x00, 0x00, 0x01
};


char adjust;
char edit_pos;
char direction;

int timing[10];
int min;
int avg;
int tot;

char i;


/* result shown in result[] array: */

add_3digit(base1, base2)
char * base1;
char * base2;
{
   int temp1, temp2;
   int temp_sum;
   int carry;

   temp1 = (*(base1+2)) & 0xff;
   temp2 = (*(base2+2)) & 0xff;

   temp_sum = temp1 + temp2;
   carry = (temp_sum) >> 8;
   result[2] = temp_sum & 0xff;

   temp1 = (*(base1+1)) & 0xff;
   temp2 = (*(base2+1)) & 0xff;

   temp_sum = temp1 + temp2 + carry;
   carry = (temp_sum) >> 8;
   result[1] = temp_sum & 0xff;

   temp1 = (*(base1)) & 0xff;
   temp2 = (*(base2)) & 0xff;

   temp_sum = temp1 + temp2 + carry;
   result[0] = temp_sum & 0xff;
}

/* result shown in result[] array: */

sub_3digit(base1, base2)
char * base1;
char * base2;
{
   int temp1, temp2;
   int temp_sum;
   int carry;

   temp1 = (*(base1+2)) & 0xff;
   temp2 = (*(base2+2)) & 0xff;

   temp_sum = temp1 - temp2;
   carry = ((temp_sum) >> 8) & 1;
   result[2] = temp_sum & 0xff;

   temp1 = (*(base1+1)) & 0xff;
   temp2 = (*(base2+1)) & 0xff;

   temp_sum = temp1 - temp2 - carry;
   carry = ((temp_sum) >> 8) & 1;
   result[1] = temp_sum & 0xff;

   temp1 = (*(base1)) & 0xff;
   temp2 = (*(base2)) & 0xff;

   temp_sum = temp1 - temp2 - carry;
   result[0] = temp_sum & 0xff;
}


show_startpos()
{
   put_hex(start_pos[0], 2, 14, START_LINE);
   put_hex(start_pos[1], 2, 16, START_LINE);
   put_hex(start_pos[2], 2, 18, START_LINE);
}

show_offset()
{
   put_hex(offset[0], 2, 14, OFFSET_LINE);
   put_hex(offset[1], 2, 16, OFFSET_LINE);
   put_hex(offset[2], 2, 18, OFFSET_LINE);
}

show_target()
{
   put_hex(target[0], 2, 14, TARGET_LINE);
   put_hex(target[1], 2, 16, TARGET_LINE);
   put_hex(target[2], 2, 18, TARGET_LINE);
}

limit_startpos()
{
   if ((result[0] < 0x00) || (result[0] >= 0x80))
   {
      result[0] = 0x00;
      result[1] = 0x00;
      result[2] = 0x00;
   }
   if (result[0] > 0x04)
   {
      result[0] = 0x04;
      result[1] = 0xff;
      result[2] = 0xff;
   }
}

limit_offset()
{
   if ((result[0] < 0x00) || (result[0] >= 0x80))
   {
      result[0] = 0x00;
      result[1] = 0x00;
      result[2] = 0x00;
   }
}


fix_offset()
{
   if (direction == DIR_FWD)
   {
      /* if (start + offset) > maximum, reset offset to (maximum - start) */

      add_3digit(start_pos, offset);

      if (result[0] > 0x04)
      {
         target[0] = 0x04;
         target[1] = 0xff;
         target[2] = 0xff;

         sub_3digit(target, start_pos);

         offset[0] = result[0];
         offset[1] = result[1];
         offset[2] = result[2];

         add_3digit(start_pos, offset);
      }
   }
   else if (direction == DIR_BACK)
   {
      /* if (start - offset) < 0, reset offset to start */

      if ( (offset[0] > start_pos[0]) || 
          ((offset[0] == start_pos[0]) && (offset[1] > start_pos[1])) ||
          ((offset[0] == start_pos[0]) && (offset[1] == start_pos[1]) && (offset[2] > start_pos[2])))
      {
         offset[0] = start_pos[0];
         offset[1] = start_pos[1];
         offset[2] = start_pos[2];
      }
      sub_3digit(start_pos, offset);
   }

   /* in either case, target needs to be set as final (start +- offset) */

   target[0] = result[0];
   target[1] = result[1];
   target[2] = result[2];
}


clear_index()
{
   put_string("      ", 14, START_UNDER);
   put_string("      ", 14, OFFSET_UNDER);
   put_string("   ", 2, DIRECTION_LINE);
   put_string("   ", 26, DIRECTION_LINE);
}

put_index()
{
   if (direction == DIR_FWD)
   {
       put_string("FORWARD ", 14, DIRECTION_LINE);
   }
   else
   {
       put_string("BACKWARD", 14, DIRECTION_LINE);
   }

   if (adjust == ADJUST_DIRECTION)
   {
       put_string(">>>", 2, DIRECTION_LINE);
       put_string("<<<", 26, DIRECTION_LINE);
   }
   else if (adjust == ADJUST_START)
   {
       put_string("      ", 14, START_UNDER);
       put_string("^", 14+edit_pos, START_UNDER);
   }
   else if (adjust == ADJUST_OFFSET)
   {
       put_string("      ", 14, OFFSET_UNDER);
       put_string("^", 14+edit_pos, OFFSET_UNDER);
   }   
}

main()
{
   set_color(0,0);
   set_color(1,511);

   for (i = 0; i < 3; i++) {
      start_pos[i] = 0;
      offset[i] = 0;
      target[i] = 0;
   }

   edit_pos = 0;
   adjust = ADJUST_START;
   direction = DIR_FWD;

   put_string("Head Seek Test", 10, TITLE_LINE);

   put_string("       Dir: ", 2, DIRECTION_LINE);
   put_string("Start  LBA: ", 2, START_LINE);
   put_string("Offset LBA: ", 2, OFFSET_LINE);
   put_string("Target LBA: ", 2, TARGET_LINE);

   show_startpos();
   show_offset();
   show_target();

   put_string("Change Values: direction pad", 2, INSTRUCT_LINE1);

   put_string("Button II = fwd; I = back", 2, INSTRUCT_LINE3);

   while(1)
   {
      put_index();
      pad = joytrg(0);

      if (adjust == ADJUST_DIRECTION)
      {
         if (pad & (JOY_LEFT | JOY_RGHT | JOY_UP | JOY_DOWN))
         {
            if (direction == DIR_FWD)
            {
               direction = DIR_BACK;
               put_index();

               /* if startpos + offset is outside limits, adjust offset */
               fix_offset();
               show_offset();
               show_target();
            }
            else
            {
               direction = DIR_FWD;
               put_index();

               /* if startpos + offset is outside limits, adjust offset */
               fix_offset();
               show_offset();
               show_target();
            }
         }
         if (pad & JOY_B)
         {
            adjust = ADJUST_START;
            clear_index();
            put_index();
         }
      }
      else if ((adjust == ADJUST_START) || (adjust == ADJUST_OFFSET))
      {
         if (pad & JOY_LEFT)
         {
            if (edit_pos > 0)
            {
               edit_pos -= 1;
               put_index();
            }
         }

         if (pad & JOY_RGHT)
         {
            if (edit_pos < 5)
            {
               edit_pos += 1;
               put_index();
            }
         }

         if (pad & JOY_UP)
         {
            if (adjust == ADJUST_START)
            {
               add_3digit(&start_pos[0], &add_num[(edit_pos*3)]);

               limit_startpos();  /* check if outside bounds */

               start_pos[0] = result[0];
               start_pos[1] = result[1];
               start_pos[2] = result[2];
               show_startpos();

               /* if startpos + offset is outside limits, adjust offset */
               fix_offset();
               show_offset();
               show_target();
            }
            else if (adjust == ADJUST_OFFSET)
            {
               add_3digit(&offset[0], &add_num[(edit_pos*3)]);

               limit_offset();

               offset[0] = result[0];
               offset[1] = result[1];
               offset[2] = result[2];

               /* if startpos + offset is outside limits, adjust offset */
               fix_offset();
               show_offset();
               show_target();
            }
         }

         if (pad & JOY_DOWN)
         {
            if (adjust == ADJUST_START)
            {
               sub_3digit(&start_pos[0], &add_num[(edit_pos*3)]);

               limit_startpos();  /* check if outside bounds */

               start_pos[0] = result[0];
               start_pos[1] = result[1];
               start_pos[2] = result[2];
               show_startpos();

               /* if startpos + offset is outside limits, adjust offset */
               fix_offset();
               show_offset();
               show_target();
            }
            else if (adjust == ADJUST_OFFSET)
            {
               sub_3digit(&offset[0], &add_num[(edit_pos*3)]);

               limit_offset();

               offset[0] = result[0];
               offset[1] = result[1];
               offset[2] = result[2];

               /* if startpos + offset is outside limits, adjust offset */
               fix_offset();
               show_offset();
               show_target();
            }
         }

         if (pad & JOY_B)  /* this is button II */
         {
            if (adjust == ADJUST_START)
            {
               adjust = ADJUST_OFFSET;

               clear_index();
               put_index();
            }
            else if (adjust == ADJUST_OFFSET)
            {
               clear_index();
               adjust = ADJUST_OFFSET;

               for (i = 0; i < 10; i++)
               {
                  put_string("     ", 5, READING_LINE+i);
               }
               put_string("          ", 18, MIN_LINE);
               put_string("          ", 18, AVG_LINE);

               /* run test */
               min = 32767;
               tot = 0;
            
/*               for (i = 0; i < 10; i++)
               {
                  put_string("Move to initial position", 2, STAT_LINE);
*/
                  /*initial head position seek */

/*
                  rec_h = start_pos[0];
                  rec_m = start_pos[1];
                  rec_l = start_pos[2];

                  cd_read_sector();

                  put_string("Random delay            ", 2, STAT_LINE);
                  vsync(random(55)+5);               
*/
                  put_string("Seeking...              ", 2, STAT_LINE);

                  rec_h = target[0];
                  rec_m = target[1];
                  rec_l = target[2];
/*
                  vsync(0);
                  clock_reset();
*/
                  /* Seek */

                  beepsetup();
                  beepon();
                  beeplow();

                  cd_read_sector();
                  beephigh();

		  for (i = 0; i < 60; i++) {
                     vsync(0);
		  }
                  beepoff();
                  cd_stop();

		  /* cd_stop_play(); */  /* call cd_pause and stop tone */
		  start_pos[0] = target[0];
		  start_pos[1] = target[1];
		  start_pos[2] = target[2];

		  offset[0] = 0;
		  offset[1] = 0;
		  offset[2] = 0;

		  show_startpos();
		  show_offset();
		  show_target();
/*
                  timing[i] = (clock_mm() * 3600) + (clock_ss() * 60) + clock_tt();
                  if (timing[i] < min)
                  {
                     min = timing[i];
                  }

                  tot = tot + timing[i];
                  put_number(timing[i], 5, 5, READING_LINE+i);
*/
		  /* re-establish starting point */

/*               } */
/*
               avg = tot / 10;
*/
               put_string("                        ", 2, STAT_LINE);
/*
               put_string("Min: ", 18, MIN_LINE);
               put_number(min, 5, 23, MIN_LINE);

               put_string("Avg: ", 18, AVG_LINE);
               put_number(avg, 5, 23, AVG_LINE);
*/
            }
         }


         if (pad & JOY_A)
         {
            if (adjust == ADJUST_OFFSET)
            {
               adjust = ADJUST_START;

               clear_index();
               put_index();
            }
            else if (adjust == ADJUST_START)
            {
               adjust = ADJUST_DIRECTION;

               clear_index();
               put_index();
            }
         }
      }

      vsync(0);
   }

   return;
}
