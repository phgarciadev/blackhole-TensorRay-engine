/**
 * @file assert.h
 * @brief Assertion macro para o gui-framework
 *
 * gui-framework tem seu próprio assert porque não pode depender de math/.
 * Se math/ mudar o assert, gui-framework não é afetado. Desacoplamento total.
 */

#ifndef BHS_GUI_FRAMEWORK_ASSERT_H
#define BHS_GUI_FRAMEWORK_ASSERT_H

#include <assert.h>

/*
 * BHS_ASSERT - Assertion para gui-framework
 *
 * Usa o assert padrão do C, mas encapsulado pra
 * eventual customização (log, recovery, etc).
 */
#define BHS_ASSERT(expr) assert(expr)

#endif /* BHS_gui-framework_ASSERT_H */
