/* Bob Generated JavaScript Runtime */
"use strict";

function __bob_get(selector, evt) {
    if (evt && evt.currentTarget) {
        if (evt.currentTarget.id === selector || evt.currentTarget.tagName.toLowerCase() === selector.toLowerCase()) {
            return [evt.currentTarget];
        }
    }
    if (selector.includes('!')) {
        let [tag, excl] = selector.split('!');
        return Array.from(document.querySelectorAll(tag || '*')).filter(el => !el.classList.contains(excl) && el.tagName.toLowerCase() !== excl.toLowerCase() && el.id !== excl);
    }
    let el = document.getElementById(selector);
    if (el) return [el];
    return Array.from(document.querySelectorAll(selector));
}

function __bob_resolve(selector, evt) {
    if (selector === "root" || selector === "document" || selector === "html") {
        return [document.documentElement];
    }
    return __bob_get(selector, evt);
}

function __bob_read(selector, evt) {
    let list = __bob_get(selector, evt);
    if (list.length > 0) return list[0].value !== undefined ? list[0].value : (list[0].textContent || '');
    return '';
}

function __bob_update(selector, val, evt) {
    let list = __bob_get(selector, evt);
    list.forEach(el => {
        if (el.tagName.toLowerCase() === 'input') {
            el.value = (val === undefined) ? '' : val;
        } else {
            el.textContent = (val === undefined) ? '' : val;
        }
    });
}

function __bob_show(selector, evt) {
    let list = __bob_get(selector, evt);
    list.forEach(el => { el.style.display = ''; });
}

function __bob_hide(selector, evt) {
    let list = __bob_get(selector, evt);
    list.forEach(el => { el.style.display = 'none'; });
}

function __bob_add_class(selector, cls, evt) {
    let list = __bob_resolve(selector, evt);
    list.forEach(el => el.classList.add(cls));
}

function __bob_remove_class(selector, cls, evt) {
    let list = __bob_resolve(selector, evt);
    list.forEach(el => el.classList.remove(cls));
}

function __bob_toggle_class(selector, cls, evt) {
    let list = __bob_resolve(selector, evt);
    list.forEach(el => el.classList.toggle(cls));
}

function __bob_bind_click(target, handler) {
    let targets = __bob_get(target, null);
    targets.forEach(el => {
        el.addEventListener('click', handler);
    });
}

// State Variables
let visitor_name = undefined;
let is_input_open = 0;

window.addEventListener('DOMContentLoaded', function() {
    const _event = null;
    __bob_bind_click("hello_btn", function(_event) {
        if ((is_input_open === 0)) {
            __bob_show("name_box", _event);
            __bob_update("hello_btn", "Confirm", _event);
            is_input_open = 1;
        } else {
            visitor_name = __bob_read("name_box", _event);
            if ((visitor_name === "")) {
                __bob_update("greeting", "Hello, stranger.", _event);
            } else {
                __bob_update("greeting", (("Hello, " + visitor_name) + "."), _event);
            }
            __bob_hide("name_box", _event);
            __bob_update("hello_btn", "Say hello", _event);
            is_input_open = 0;
        }
    });
});
