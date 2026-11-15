
#ifndef CONTEXT_MENU_H
#define CONTEXT_MENU_H

class ContextMenu {
public:
    enum class Action {
        Open,
        Copy,
        Cut,
        Write,       // New option
        Rename,
        Delete,
        Properties,
        Cancel
    };

    Action show(bool isDirectory);
};

#endif
