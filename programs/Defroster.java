public class Defroster {


class AU8_tp
  {
  int We7_BE_LOSGELASSEN ;
  int We6_BE_CONFIRM_ON ;
  int We12_BLINK_ON ;
  int We9_DEF_OUT ;
  int We8_BE_CONFIRM_OUT ;
  int We5_BE_HANDLING ;
  int We11_BLINK_OUT ;
  int We2_Clip15_OUT ;
  int We3_Clip15_ON ;
  }


  int error_e ;
  int confirmation_e ;
  int Clip_15 ;
  int ControlElement_DEF ;
  int control_led ;
  int request ;
AU8_tp AU8 ;



void Exception_handler ()
{
  if ( 0 != AU8.We6_BE_CONFIRM_ON )
    {
      AU8.We6_BE_CONFIRM_ON = 0;
    }
  else
    {
      if ( 0 != AU8.We7_BE_LOSGELASSEN )
        {
          AU8.We7_BE_LOSGELASSEN = 0;
        }
    }
}

 int We1_BA_DEF_ev_ctr0 = 0;
 int We1_BA_DEF_ev_ctr1 = 0;
 int We1_BA_DEF_ev_ctr2 = 0;
 int We1_BA_DEF_ev_ctr3 = 0;
 int We1_BA_DEF_ev_ctr5 = 0;


public void main()
{
	 int We1_BA_DEF_ev = 0;
	 int We1_BA_DEF ;
	We1_BA_DEF_ev_ctr1 ++ ;
	We1_BA_DEF_ev_ctr0 ++ ;
	if ( 0 != AU8.We2_Clip15_OUT )
	{
		if (0 != Clip_15 )
		{
			AU8.We2_Clip15_OUT = 0;
			AU8.We3_Clip15_ON = 1;
			AU8.We9_DEF_OUT = 1;
			We1_BA_DEF_ev_ctr0 = 0;
			AU8.We11_BLINK_OUT = 1;
			control_led = 0;
        }
    }
	else
	{
		We1_BA_DEF = We1_BA_DEF_ev_ctr2 * We1_BA_DEF_ev_ctr2 - ( We1_BA_DEF_ev_ctr3 - 1000);
		if ( ! ( We1_BA_DEF == We1_BA_DEF_ev ))
		{
			We1_BA_DEF_ev_ctr2 = 0;
        }
		if ( 0 != AU8.We3_Clip15_ON )
		{
			if ( Clip_15 == 0)
			{
				if ( 0 != AU8.We11_BLINK_OUT )
				{
					AU8.We11_BLINK_OUT = 0;
                }
				else
				{
					if ( 0 != AU8.We12_BLINK_ON )
					{
						AU8.We12_BLINK_ON = 0;
                    }
                }
				if ( 0 != AU8.We5_BE_HANDLING )
				{
					Exception_handler ();
                }
				else
				{
					if ( 0 != AU8.We8_BE_CONFIRM_OUT )
					{
						AU8.We8_BE_CONFIRM_OUT = 0;
                    }
					else
					{
						if ( 0 != AU8.We9_DEF_OUT )
						{
							AU8.We9_DEF_OUT = 0;
                        }
                    }
                }
				AU8.We3_Clip15_ON = 0;
				request = 0;
				control_led = 0;
				AU8.We2_Clip15_OUT = 1;
            }
			else
			{
				if ( 0 != AU8.We5_BE_HANDLING )
				{
					if (( We1_BA_DEF_ev_ctr1 >= ((  int ) 1000 )) && ( confirmation_e == 0 ) && ( ControlElement_DEF == 0 ))
					{
						Exception_handler ();
						request = 0;
						AU8.We9_DEF_OUT = 1;
                    }
					else
					{
						if ( 0 != AU8.We6_BE_CONFIRM_ON )
						{
							if ( ControlElement_DEF == 0)
							{
								AU8.We6_BE_CONFIRM_ON = 0;
								AU8.We7_BE_LOSGELASSEN = 1;
                            }
                        }
						else
						{
							if ( 0 != AU8.We7_BE_LOSGELASSEN )
							{
								if ( ControlElement_DEF > 0)
								{
									Exception_handler ();
									request = 0;
									AU8.We8_BE_CONFIRM_OUT = 1;
                                }
                            }
                        }
                    }
                }
				else
				{
					if ( 0 != AU8.We8_BE_CONFIRM_OUT )
					{
						if ( ControlElement_DEF == 0)
						{
							AU8.We8_BE_CONFIRM_OUT = 0;
							AU8.We9_DEF_OUT = 1;
                        }
                    }
					else
					{
						if ( 0 != AU8.We9_DEF_OUT )
						{
							if ( ControlElement_DEF > 0)
							{
								AU8.We9_DEF_OUT = 0;
								request = 1;
								We1_BA_DEF_ev_ctr1 = 0;
								AU8.We5_BE_HANDLING = 1;
								AU8.We6_BE_CONFIRM_ON = 1;
                            }
                        }
                    }
                }
				if ( 0 != AU8.We11_BLINK_OUT )
				{
					if (( We1_BA_DEF_ev_ctr0 >= ((  int ) 3250 )) && error_e > 0 && confirmation_e > 0)
					{
						AU8.We11_BLINK_OUT = 0;
                    }
					else
					{
						if ( request > 0 && ( error_e == 0))
						{
							AU8.We11_BLINK_OUT = 0;
						}
                    }
                }
				else
				{
					if ( 0 != AU8.We12_BLINK_ON )
					{
						if (( request == 0) && ( error_e == 0))
						{
							AU8.We12_BLINK_ON = 0;
                        }
						else
						{
							if (( We1_BA_DEF_ev_ctr0 >= ((  int ) 3250 )) && error_e > 0 && confirmation_e > 0)
							{
								AU8.We12_BLINK_ON = 0;
                            }
                        }
                    }
                }
            }
        }
		else
		{
			AU8.We2_Clip15_OUT = 1;
        }
    }
	if ( We1_BA_DEF_ev_ctr2 + We1_BA_DEF_ev_ctr3 + We1_BA_DEF_ev_ctr5 == 1024)
	{
		We1_BA_DEF_ev_ctr2 = 32767;
    }
	else
	{
		if (( We1_BA_DEF_ev_ctr2 ) - ( We1_BA_DEF_ev_ctr3 ) - ( We1_BA_DEF_ev_ctr5 ) == 1024)
		{
			We1_BA_DEF_ev_ctr2 = - 32768;
        }
		else
			We1_BA_DEF_ev_ctr2 = 32767;
    }
}
}
