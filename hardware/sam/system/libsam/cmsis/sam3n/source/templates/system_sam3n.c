/*! \file *********************************************************************
 *
 * \brief Provides the low-level initialization functions that called 
 * on chip startup.
 *
 * $asf_license$
 *
 * \par Purpose
 *
 * This file provides basic support for Cortex-M processor based 
 * microcontrollers.
 *
 * \author               Atmel Corporation: http://www.atmel.com \n
 *                       Support and FAQ: http://support.atmel.no/
 *
 ******************************************************************************/

#include "system_sam3n.h"
#include "sam3n.h"

/* @cond 0 */
/**INDENT-OFF**/
#ifdef __cplusplus
extern "C" {
#endif
/**INDENT-ON**/
/* @endcond */

/* Clock settings (48MHz) */
#define SYS_BOARD_OSCOUNT   (CKGR_MOR_MOSCXTST(0x8))
#define SYS_BOARD_PLLR		(CKGR_PLLR_STUCKTO1 \
							| CKGR_PLLR_MUL(0x3) \
							| CKGR_PLLR_PLLCOUNT(0x3f) \
							| CKGR_PLLR_DIV(0x1))
#define SYS_BOARD_MCKR      (PMC_MCKR_PRES_CLK | PMC_MCKR_CSS_PLL_CLK)

/* Clock Definitions */
#define SYS_FREQ_XTAL_32K      	(32768UL)		/* External 32K crystal frequency */
#define SYS_FREQ_XTAL_XTAL12M	(12000000UL)	/* External 12M crystal frequency */

#define SYS_FREQ_FWS_0	(21000000UL)	/* Maximum operating frequency when FWS is 0 */
#define SYS_FREQ_FWS_1	(32000000UL)	/* Maximum operating frequency when FWS is 1 */
#define SYS_FREQ_FWS_2	(48000000UL)	/* Maximum operating frequency when FWS is 2 */


#define SYS_CKGR_MOR_KEY_VALUE	CKGR_MOR_KEY(0x37U) /* Key to unlock MOR register */

/* FIXME: should be generated by sock */
uint32_t SystemCoreClock = CHIP_FREQ_MAINCK_RC_4MHZ;

/**
 * \brief Setup the microcontroller system.
 * Initialize the System and update the SystemFrequency variable.
 */
void SystemInit(void)
{
	/* Set 3 FWS for Embedded Flash Access */
	EFC->EEFC_FMR = EEFC_FMR_FWS(CHIP_FLASH_WAIT_STATE);

	/* Initialize main oscillator */
	if (!(PMC->CKGR_MOR & CKGR_MOR_MOSCSEL)) {
		PMC->CKGR_MOR = SYS_CKGR_MOR_KEY_VALUE | SYS_BOARD_OSCOUNT | 
			                     CKGR_MOR_MOSCRCEN | CKGR_MOR_MOSCXTEN;
		while (!(PMC->PMC_SR & PMC_SR_MOSCXTS)) {
		}
	}

	/* Switch to 3-20MHz Xtal oscillator */
	PMC->CKGR_MOR = SYS_CKGR_MOR_KEY_VALUE | SYS_BOARD_OSCOUNT | 
	CKGR_MOR_MOSCRCEN | CKGR_MOR_MOSCXTEN | CKGR_MOR_MOSCSEL;

	while (!(PMC->PMC_SR & PMC_SR_MOSCSELS)) {
	}
	PMC->PMC_MCKR = (PMC->PMC_MCKR & ~(uint32_t)PMC_MCKR_CSS_Msk) | 
		                    PMC_MCKR_CSS_MAIN_CLK;
	while (!(PMC->PMC_SR & PMC_SR_MCKRDY)) {
	}

	/* Initialize PLL */
	PMC->CKGR_PLLR = SYS_BOARD_PLLR;
	while (!(PMC->PMC_SR & PMC_SR_LOCK)) {
	}

	/* Switch to main clock */
	PMC->PMC_MCKR = (SYS_BOARD_MCKR & ~PMC_MCKR_CSS_Msk) | 
	PMC_MCKR_CSS_MAIN_CLK;
	while (!(PMC->PMC_SR & PMC_SR_MCKRDY)) {
	}

	/* Switch to PLLA */
	PMC->PMC_MCKR = SYS_BOARD_MCKR;
	while (!(PMC->PMC_SR & PMC_SR_MCKRDY)) {
	}

	SystemCoreClock = CHIP_FREQ_CPU_MAX;
}

/** 
 * Initialize the flash and watchdog setting .
 */
void set_flash_and_watchdog(void)
{
	/* Set FWS for Embedded Flash Access according operating frequency*/
	if(SystemCoreClock < SYS_FREQ_FWS_0){
		EFC->EEFC_FMR = EEFC_FMR_FWS(0);
	}else if(SystemCoreClock < SYS_FREQ_FWS_1){
		EFC->EEFC_FMR = EEFC_FMR_FWS(1);
	}else if(SystemCoreClock < SYS_FREQ_FWS_2){
		EFC->EEFC_FMR = EEFC_FMR_FWS(2);
	}else{
		EFC->EEFC_FMR = EEFC_FMR_FWS(3);
	}	
	
#ifndef CONFIG_KEEP_WATCHDOG_AFTER_INIT 
	/*Disable the watchdog */
	WDT->WDT_MR = WDT_MR_WDDIS;
#endif
}

void SystemCoreClockUpdate(void)
{
	/* Determine clock frequency according to clock register values */
	switch (PMC->PMC_MCKR & PMC_MCKR_CSS_Msk) {
	case PMC_MCKR_CSS_SLOW_CLK:	/* Slow clock */
		if (SUPC->SUPC_SR & SUPC_SR_OSCSEL) {
			SystemCoreClock = SYS_FREQ_XTAL_32K;
		} else {
			SystemCoreClock = CHIP_FREQ_SLCK_RC;
		}
		break;
	case PMC_MCKR_CSS_MAIN_CLK:	/* Main clock */
		if (PMC->CKGR_MOR & CKGR_MOR_MOSCSEL) {
			SystemCoreClock = SYS_FREQ_XTAL_XTAL12M;
		} else {
			SystemCoreClock = CHIP_FREQ_MAINCK_RC_4MHZ;

			switch (PMC->CKGR_MOR & CKGR_MOR_MOSCRCF_Msk) {
			case CKGR_MOR_MOSCRCF_4MHz:
				break;
			case CKGR_MOR_MOSCRCF_8MHz:
				SystemCoreClock *= 2U;
				break;
			case CKGR_MOR_MOSCRCF_12MHz:
				SystemCoreClock *= 3U;
				break;
			default:
				break;
			}
		}
		break;
	case PMC_MCKR_CSS_PLL_CLK:	/* PLL clock */
		if (PMC->CKGR_MOR & CKGR_MOR_MOSCSEL) {
			SystemCoreClock = SYS_FREQ_XTAL_XTAL12M;
		} else {
			SystemCoreClock = CHIP_FREQ_MAINCK_RC_4MHZ;

			switch (PMC->CKGR_MOR & CKGR_MOR_MOSCRCF_Msk) {
			case CKGR_MOR_MOSCRCF_4MHz:
				break;
			case CKGR_MOR_MOSCRCF_8MHz:
				SystemCoreClock *= 2U;
				break;
			case CKGR_MOR_MOSCRCF_12MHz:
				SystemCoreClock *= 3U;
				break;
			default:
				break;
			}
		}
		SystemCoreClock *= ((((PMC->CKGR_PLLR) & CKGR_PLLR_MUL_Msk) >> 
			                         CKGR_PLLR_MUL_Pos) + 1U);
		SystemCoreClock /= ((((PMC->CKGR_PLLR) & CKGR_PLLR_DIV_Msk) >> 
			                         CKGR_PLLR_DIV_Pos));
		break;
	}

	if ((PMC->PMC_MCKR & PMC_MCKR_PRES_Msk) == PMC_MCKR_PRES_CLK_3) {
		SystemCoreClock /= 3U;
	} else { 
		SystemCoreClock >>= ((PMC->PMC_MCKR & PMC_MCKR_PRES_Msk) >> 
			                           PMC_MCKR_PRES_Pos);
	}
}

/* @cond 0 */
/**INDENT-OFF**/
#ifdef __cplusplus
}
#endif
/**INDENT-ON**/
/* @endcond */
