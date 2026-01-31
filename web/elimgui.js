/*
 * elimgui WASM loader
 *
 * Loads the WASM module and sets up canvas/input handling.
 */

async function loadElimgui(wasmPath) {
    const canvas = document.getElementById('canvas');
    const ctx = canvas.getContext('2d');

    // Import JAClibc's loadWASM base functionality
    // For now, minimal implementation

    const imports = {
        env: {
            // Memory
            memory: new WebAssembly.Memory({ initial: 256 }),

            // Console
            console_log: (ptr, len) => {
                // TODO: Read string from memory
                console.log('elimgui:', ptr, len);
            }
        }
    };

    try {
        const response = await fetch(wasmPath);
        const bytes = await response.arrayBuffer();
        const { instance } = await WebAssembly.instantiate(bytes, imports);

        // Call js_start if exported
        if (instance.exports.js_start) {
            instance.exports.js_start();
        }

        // Set up frame loop
        function frame() {
            if (instance.exports.frame) {
                instance.exports.frame();
            }
            requestAnimationFrame(frame);
        }

        requestAnimationFrame(frame);

        return instance;
    } catch (err) {
        console.error('Failed to load elimgui:', err);
        throw err;
    }
}
