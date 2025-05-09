#include <QApplication>
#include <QMainWindow>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QTextEdit>
#include <QDir>
#include <algorithm>
#include "ui_mainwindow.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr) : QMainWindow(parent)
    {
        ui.setupUi(this);
        connect(ui.generate, &QPushButton::clicked, this, &MainWindow::onGenerateClicked);
        
        // Set default values
        ui.textEdit_1->setPlainText("return");
        ui.textEdit_2->setPlainText("while");
        ui.textEdit_3->setPlainText("print");
        ui.textEdit_4->setPlainText("get()");
        ui.textEdit_5->setPlainText("else");
        ui.textEdit_6->setPlainText("sqrt");
        ui.textEdit_7->setPlainText("func");
        ui.textEdit_8->setPlainText("call");
        ui.textEdit_9->setPlainText("diff");
        ui.textEdit_10->setPlainText("for");
        ui.textEdit_11->setPlainText("sin");
        ui.textEdit_12->setPlainText("cos");
        ui.textEdit_13->setPlainText("log");
        ui.textEdit_14->setPlainText("do");
        ui.textEdit_15->setPlainText("..");
        ui.textEdit_16->setPlainText("if");
        ui.textEdit_17->setPlainText("==");
        ui.textEdit_18->setPlainText("!=");
        ui.textEdit_19->setPlainText("++");
        ui.textEdit_20->setPlainText("--");
        ui.textEdit_21->setPlainText("&&");
        ui.textEdit_22->setPlainText("||");
        ui.textEdit_23->setPlainText("<=");
        ui.textEdit_24->setPlainText(">=");
        ui.textEdit_25->setPlainText("&");
        ui.textEdit_26->setPlainText("|");
        ui.textEdit_27->setPlainText("^");
        ui.textEdit_28->setPlainText("+");
        ui.textEdit_29->setPlainText("-");
        ui.textEdit_30->setPlainText("*");
        ui.textEdit_31->setPlainText("/");
        ui.textEdit_32->setPlainText("(");
        ui.textEdit_33->setPlainText(")");
        ui.textEdit_34->setPlainText("{");
        ui.textEdit_35->setPlainText("}");
        ui.textEdit_36->setPlainText("^");
        ui.textEdit_37->setPlainText("=");
        ui.textEdit_38->setPlainText("<");
        ui.textEdit_39->setPlainText(">");
        ui.textEdit_40->setPlainText("\\n");
    }

private slots:
    void onGenerateClicked()
    {

        QFile file("tokens.h");
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QMessageBox::critical(this, "Error", "Could not create tokens.h in build directory!");
            return;
        }

        QTextStream out(&file);
        
        // Write header
        out << "#ifndef TOKENS_H\n"
            << "#define TOKENS_H\n\n"
            << "#include <stdio.h>\n"
            << "#include \"../../General/programTree/tree.h\"\n\n"
            << "union data_u\n"
            << "{\n"
            << "    double num;\n"
            << "    char* var;\n"
            << "};\n\n"
            << "struct node_t\n"
            << "{\n"
            << "    types type;\n"
            << "    data_u data;\n"
            << "    node_t* left;\n"
            << "    node_t* right;\n"
            << "};\n\n"
            << "struct token_map_t {\n"
            << "    const char* str;\n"
            << "    size_t len;\n"
            << "    enum types type;\n"
            << "};\n\n"
            << "const size_t c_token_mapLength = 41;\n\n"
            << "static const token_map_t token_map[] = \n"
            << "{\n";

        // Define token entries with their types
        struct TokenEntry {
            QString value;
            const char* type;
            int index;
        };

        TokenEntry entries[41];
        const char* types[] = {
            "ND_RET", "ND_WH", "ND_PR", "ND_GET", "ND_EL", "ND_SQRT", "ND_FUN", "ND_FUNCALL",
            "ND_GETDIFF", "ND_FOR", "ND_SIN", "ND_COS", "ND_LOG", "ND_DOWH", "ND_FORDD",
            "ND_IF", "ND_ISEQ", "ND_NISEQ", "ND_PRADD", "ND_PRSUB", "ND_AND", "ND_OR",
            "ND_LSE", "ND_ABE", "ND_BITAND", "ND_BITOR", "ND_XOR", "ND_ADD", "ND_SUB",
            "ND_MUL", "ND_DIV", "ND_LCIB", "ND_RCIB", "ND_LCUB", "ND_RCUB", "ND_POW",
            "ND_EQ", "ND_LS", "ND_AB", "ND_SEP", "ND_ERR"
        };

        // Get values from text edits
        entries[0].value = ui.textEdit_1->toPlainText();
        entries[1].value = ui.textEdit_2->toPlainText();
        entries[2].value = ui.textEdit_3->toPlainText();
        entries[3].value = ui.textEdit_4->toPlainText();
        entries[4].value = ui.textEdit_5->toPlainText();
        entries[5].value = ui.textEdit_6->toPlainText();
        entries[6].value = ui.textEdit_7->toPlainText();
        entries[7].value = ui.textEdit_8->toPlainText();
        entries[8].value = ui.textEdit_9->toPlainText();
        entries[9].value = ui.textEdit_10->toPlainText();
        entries[10].value = ui.textEdit_11->toPlainText();
        entries[11].value = ui.textEdit_12->toPlainText();
        entries[12].value = ui.textEdit_13->toPlainText();
        entries[13].value = ui.textEdit_14->toPlainText();
        entries[14].value = ui.textEdit_15->toPlainText();
        entries[15].value = ui.textEdit_16->toPlainText();
        entries[16].value = ui.textEdit_17->toPlainText();
        entries[17].value = ui.textEdit_18->toPlainText();
        entries[18].value = ui.textEdit_19->toPlainText();
        entries[19].value = ui.textEdit_20->toPlainText();
        entries[20].value = ui.textEdit_21->toPlainText();
        entries[21].value = ui.textEdit_22->toPlainText();
        entries[22].value = ui.textEdit_23->toPlainText();
        entries[23].value = ui.textEdit_24->toPlainText();
        entries[24].value = ui.textEdit_25->toPlainText();
        entries[25].value = ui.textEdit_26->toPlainText();
        entries[26].value = ui.textEdit_27->toPlainText();
        entries[27].value = ui.textEdit_28->toPlainText();
        entries[28].value = ui.textEdit_29->toPlainText();
        entries[29].value = ui.textEdit_30->toPlainText();
        entries[30].value = ui.textEdit_31->toPlainText();
        entries[31].value = ui.textEdit_32->toPlainText();
        entries[32].value = ui.textEdit_33->toPlainText();
        entries[33].value = ui.textEdit_34->toPlainText();
        entries[34].value = ui.textEdit_35->toPlainText();
        entries[35].value = ui.textEdit_36->toPlainText();
        entries[36].value = ui.textEdit_37->toPlainText();
        entries[37].value = ui.textEdit_38->toPlainText();
        entries[38].value = ui.textEdit_39->toPlainText();
        // entries[39].value = ui.textEdit_40->toPlainText();

        // Set types and indices
        for (int i = 0; i < 41; i++) {
            entries[i].type = types[i];
            entries[i].index = i;
        }

        // Sort entries by string length
        std::sort(entries, entries + 41, [](const TokenEntry& a, const TokenEntry& b) {
            return a.value.length() > b.value.length();
        });

        // Write each token entry
        for (int i = 0; i < 41; i++) {
            QString value = entries[i].value.trimmed();
            if (value.isEmpty()) {
                value = "nullptr";
            } else {
                value = "\"" + value + "\"";
            }
            
            out << "    {" << value << ", " 
                << (value == "nullptr" ? "0" : QString::number(entries[i].value.length())) << ", "
                << entries[i].type << "}";
            
            if (i < 40) {
                out << ",";
            }
            out << "\n";
        }

        // Write footer
        out << "};\n\n"
            << "struct toksInfo_t\n"
            << "{\n"
            << "    node_t* tokens;\n"
            << "    size_t ind;\n"
            << "    size_t cap;\n"
            << "};\n\n"
            << "#endif\n";

        file.close();
        QMessageBox::information(this, "Success", "New tokens.h has been created successfully!");
    }

private:
    Ui::MainWindow ui;
};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    MainWindow window;
    window.show();
    return app.exec();
}

#include "main.moc"