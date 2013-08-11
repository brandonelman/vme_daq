#include "V812.h" 

CVErrorCodes write_to_v812(uint32_t vme_addr, uint32_t data)
{
  CVDataWidth data_size = cvD16;
  CVAddressModifier addr_mode = cvA32_U_DATA;
  vme_addr = vme_addr + V812_BASE_ADDRESS;
  return CAENVME_WriteCycle(handle, vme_addr, &data, addr_mode, data_size);
}  

CVErrorCodes set_channel_threshold(int ch_num, int threshold)
{
  uint32_t vme_addr; 

  switch(ch_num)
  {
    case 0: vme_addr = V812_THRESHOLD_CH0; 
            break;
    case 1: vme_addr = V812_THRESHOLD_CH1; 
            break;
    case 2: vme_addr = V812_THRESHOLD_CH2; 
            break;
    case 3: vme_addr = V812_THRESHOLD_CH3; 
            break;
    case 4: vme_addr = V812_THRESHOLD_CH4; 
            break;
    case 5: vme_addr = V812_THRESHOLD_CH5; 
            break;
    case 6: vme_addr = V812_THRESHOLD_CH6; 
            break;
    case 7: vme_addr = V812_THRESHOLD_CH7; 
            break;
    case 8: vme_addr = V812_THRESHOLD_CH8; 
            break;
    case 9: vme_addr = V812_THRESHOLD_CH9; 
            break;
    case 10: vme_addr = V812_THRESHOLD_CH10; 
             break;
    case 11: vme_addr = V812_THRESHOLD_CH11;
             break;
    case 12: vme_addr = V812_THRESHOLD_CH12;
             break;
    case 13: vme_addr = V812_THRESHOLD_CH13;
             break;
    case 14: vme_addr = V812_THRESHOLD_CH14;
             break;
    case 15: vme_addr = V812_THRESHOLD_CH15;
             break;

    default: printf("Bad choice of channel!");
             return cvInvalidParam; 
  }         

  return write_to_v812(vme_addr, threshold);
}

CVErrorCodes set_output_width(int ch_section, int set_count)
{
  uint32_t vme_addr; 

  if (ch_section == 1)
    vme_addr = V812_OUTPUT_WIDTH_1;  

  else if(ch_section == 2)
    vme_addr = V812_OUTPUT_WIDTH_2;
  
  else
  {
    printf("Invalid channel section! Choose either 1 or 2!");
    return cvInvalidParam;
  }
  
  return write_to_v812(vme_addr, set_count);
}

CVErrorCodes set_dead_time(int ch_section, int set_count)
{
  uint32_t vme_addr; 

  if (ch_section == 1)
    vme_addr = V812_DEAD_TIME_1; 

  else if(ch_section == 2)
    vme_addr = V812_DEAD_TIME_2;
  
  else
  {
    printf("Invalid channel section! Choose either 1 or 2!");
    return cvInvalidParam;
  }
  
  return write_to_v812(vme_addr, set_count);
}

CVErrorCodes set_majority_threshold(int maj_lev) 
{
  uint32_t vme_addr = V812_MAJORITY_THRESHOLD; 
  int majority_treshold = (int)((maj_lev*50.0-25.0)/4.0);

  return write_to_v812(vme_addr, majority_treshold);
}

CVErrorCodes send_test_pulse(void)
{
  return write_to_v812(V812_TEST_PULSE, 1);
}

