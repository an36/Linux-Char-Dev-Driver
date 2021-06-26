 *										*
 *	Author: Abdullah Almarzouq (an36@pdx.edu)				*
 *	5/22/20									*
 *	ECE373 | Homework 5							*
 *										*
 *	hw5.c: This program works as a userspace driver that reads		*
 *		and modifies the LEDCTL register to turn the LEDs on		*
 *		and off. It accesses the LEDCTL register by memory		*
 *		mapping the e1000 driver.					*
 *										*
 *	Note(s): 								*
 *		- All of the functions were supplied by ledmon.c.		*
 *		  (http://web.cecs.pdx.edu/~peterw/ece373/ledmon/)		*
 *										*
 *		- The Makefile will create an executable called hw5.		*
 *										*
 *		- typescript_hw5.txt: shows the output/results of executing	*
 *		  hw5.c.							*
 *										*
 *		- typescript_ledmon: shows the output of ledmon when 		*
 *		  executing hw5.c.						*
 *										*
 *	Arguments:								*
 *		-s	<bus:slot.function>					*				
 *										*
 ********************************************************************************

	Execute:

		>$ sudo ./<prog_name>  -s  <bus:slot.functions>


	Example:

		>$ sudo ./hw5 -s 00:03.0
		
		LEDCTL value: 117867266
		LED0 & LED2 on for 2 seconds
		All LEDs off for 2 seconds
		Looping around LED2, LED1 and LED0: LOOP[1]
		Looping around LED2, LED1 and LED0: LOOP[2]
		Looping around LED2, LED1 and LED0: LOOP[3]
		Looping around LED2, LED1 and LED0: LOOP[4]
		Looping around LED2, LED1 and LED0: LOOP[5]
		Restoring LEDCTL register...
		Good Packets Recieved statics register: 0


	Note:
		Since this is a userspace driver, we do have to unload the current
		e1000 driver because we do not need to load our personal kernel
		module driver.  To elaborate, we are not changing anything in
		kernel space, therefore, we don't need to unload the current
		driver.