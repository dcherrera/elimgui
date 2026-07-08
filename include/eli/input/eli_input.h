/**
 * @file eli_input.h
 * @brief Aggregator for the elimgui input category: mouse, keyboard, text queue,
 *        shortcuts + clipboard, and the backend event adders / per-frame update,
 *        included in dependency order.
 *
 * Include this to get the whole input surface:
 *     #include <eli/input/eli_input.h>
 *
 * @status Phase 4 input aggregator in use.
 * @issues None
 * @todo None
 */
#ifndef ELI_INPUT_ELI_INPUT_H
#define ELI_INPUT_ELI_INPUT_H

#include "eli_input_mouse.h"
#include "eli_input_keyboard.h"
#include "eli_input_text.h"
#include "eli_input_shortcut.h"
#include "eli_input_backend.h"

#endif /* ELI_INPUT_ELI_INPUT_H */
