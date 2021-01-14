#define bool int
#define true 1
#define false 0 
bool ignition;     //engine off
double throttle;        //throttle setting 0 .. 10, controlling the amount of gaz getting to the engine
int speed;                //speed 0 .. 120 mph!, actual simulated car speed
int distance;             //the crossed distance since starting the engine 
int brakepedal;           //brake setting 0..10, allowing different levels of braking
bool engine;				  //car engine, null when the car is stopped 
	
static int maxSpeed = 120;
static double maxThrottle = 10.0;
static int maxBrake = 10;
static double airResistance = 12.5;  //inverse air resistance factor
static int ticksPerSecond = 5;

	// -----------
	// Constructor
	// -----------    

void CarSimulator() 
 {
     //super();
 }
   
    /**
     * Starting the car engine. It creates a thread for the engine.
     */
    
  
bool engineOn(bool engine)
{
    ignition = true;
    if (engine==false) 
    {
       	engine = true;
    }
    return engine;
 }

    /**
     * Stopping the car engine. Resets all parameters.
     */
void  engineOff() 
{
    ignition = false;
    speed=0;
    distance=0;
    throttle=0;
    brakepedal=0;
    engine=false;
 }

    /**
     * When the engine is started, the throttle setting is incremented 
     * by 5 if not already at its maximum otherwise it is set to the maximum 
     * throttle value.
     */
 double accelerate(double throttle)
 {
	 if (throttle<(maxThrottle-5))
		 throttle =throttle+5.0;  
	 else
		 throttle=maxThrottle;
	 return throttle;
}

    /**
     * When the engine is started, the brake setting is incremented 
     * by 1 if not already at its maximum otherwise it is set to the maximum 
     * braking value.
     */    
int brake(int brakepedal) 
{
	if (brakepedal<(maxBrake-1))
		brakepedal =brakepedal+1;
	else
		brakepedal=maxBrake;
	return brakepedal;
}

    /**
     * When the engine is started, the engine thread runs until it
     * is turned off. While running, speed and distance are
     * updated every 200 milliseconds
     *
     */
int *run(double throttle, int brakepedal)
{
   	double fdist=0.0;
   	double fspeed=0.0;
   	int speed, distance;
	int *result;
	    	
   	speed=0;
   	distance=0;
    	
	//updating the speed based on the actual speed, throttle and brakepedal values
	fspeed = fspeed+((throttle - fspeed/airResistance - 2*brakepedal))/ticksPerSecond;
	if (fspeed>maxSpeed) 
		fspeed=maxSpeed;
	if (fspeed<0)
		fspeed=0;
	//updating the distance based on the calculated speed
	fdist = fdist + (fspeed/36.0)/ticksPerSecond;
	speed = (int)fspeed;
	distance=(int)fdist;
	//adjusting throttle value to account for decay;
	if (throttle>0.0) 
		throttle-=0.5/ticksPerSecond;
	if (throttle<0.0) 
		throttle=0;
   	
  	result[0]=speed;
	result[1]=distance;
   	return result;
}

    /**
     * The controller set the throttle value while the cruise control is on
     * to adjust the car speed
     */
double setThrottle(double val) 
{
    double throttle=val;
    if (throttle<0.0)
     	throttle=0.0;
    if (throttle>10.0) 
      	throttle=10.0;
    return throttle;
}

    /**
     * @return current car speed
     */
int getSpeed(int speed) 
{
    return speed;
}

    /**
     * @return current ignition state - true if the car is started, false otherwise
     */
bool getIgnition(bool ignition) 
{
	return ignition;
}
 
    /**
     * @return throttle level
     */
double getThrottle(int throttle) 
{
     return throttle;
}

    /**
     * @return current brakepedal level
     */
int getBrakepedal(int brakepedal) 
{
    return brakepedal;
}
    
    /**
     * @return current distance
     */   
int getDistance(int distance) 
{
  	return distance;
}