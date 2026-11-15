
#include "ContextMenu.h"

#include <iostream>
#include <iomanip>
#include <limits>

using namespace std;

ContextMenu::Action ContextMenu::show(bool isDirectory) {

    int choice;

    cout << "\n+--------------------------------------+\n";

    if (isDirectory) {
        cout << "|             FOLDER OPTIONS           |\n";
    }
    else {
        cout << "|              FILE OPTIONS            |\n";
    }

    cout << "+--------------------------------------+\n";

    cout << "|  1. Open "
         << left << setw(27)
         << (isDirectory ? "Folder" : "File") << "|\n";

    cout << "|  2. Copy                             |\n";
    cout << "|  3. Cut                              |\n";

    if (!isDirectory) {
        cout << "|  4. Write / Edit                     |\n";
        cout << "|  5. Rename                           |\n";
        cout << "|  6. Delete                           |\n";
        cout << "|  7. Properties                       |\n";
        cout << "|  8. Cancel                           |\n";
    }
    else {
        cout << "|  4. Rename                           |\n";
        cout << "|  5. Delete                           |\n";
        cout << "|  6. Properties                       |\n";
        cout << "|  7. Cancel                           |\n";
    }

    cout << "+--------------------------------------+\n";

    cout << " Enter your choice: ";

    if (!(cin >> choice)) {

        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');

        cout << "\n[ERROR] Invalid input.\n";

        return Action::Cancel;
    }

    cin.ignore(numeric_limits<streamsize>::max(), '\n');

    if (!isDirectory) {

        switch (choice) {

            case 1: return Action::Open;
            case 2: return Action::Copy;
            case 3: return Action::Cut;
            case 4: return Action::Write;
            case 5: return Action::Rename;
            case 6: return Action::Delete;
            case 7: return Action::Properties;
            case 8: return Action::Cancel;

            default:
                cout << "\n[ERROR] Invalid choice.\n";
                return Action::Cancel;
        }
    }
    else {

        switch (choice) {

            case 1: return Action::Open;
            case 2: return Action::Copy;
            case 3: return Action::Cut;
            case 4: return Action::Rename;
            case 5: return Action::Delete;
            case 6: return Action::Properties;
            case 7: return Action::Cancel;

            default:
                cout << "\n[ERROR] Invalid choice.\n";
                return Action::Cancel;
        }
    }
}
