/**
 * @file assert.h
 * @brief Assertion macro para o Framework
 *
 * Framework tem seu próprio assert porque não pode depender de math/.
 * Se math/ mudar o assert, framework não é afetado. Desacoplamento total.
 */

#ifndef BHS_FRAMEWORK_ASSERT_H
#define BHS_FRAMEWORK_ASSERT_H

#include <assert.h>

/*
 * BHS_ASSERT - Assertion para framework
 *
 * Usa o assert padrão do C, mas encapsulado pra
 * eventual customização (log, recovery, etc).
 */
#define BHS_ASSERT(expr) assert(expr)

#endif /* BHS_FRAMEWORK_ASSERT_H */
