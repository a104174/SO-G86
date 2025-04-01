# 🗂️ Sistema de Indexação de Documentos

Este projeto foi desenvolvido no âmbito da unidade curricular de **Sistemas Operativos**.  
Consiste num sistema cliente-servidor, implementado em **C**, que permite a **indexação, consulta e pesquisa de documentos** através de comunicação por **FIFOs com nome (named pipes)**.

## 📌 Funcionalidades Implementadas

### 📥 Adição de Documentos (`-a`)
- Permite adicionar um documento com os seguintes campos:
  - Título
  - Autores
  - Ano de publicação
  - Caminho para o ficheiro do documento
- Os metadados são guardados no ficheiro `meta_data.txt` e associados a um identificador único.

### 🔎 Consulta de Documentos (`-c`)
- Consulta os metadados de um documento a partir do seu identificador (`id`).
- Os dados apresentados incluem título, autores, ano e caminho do ficheiro.

### 📊 Contagem de Linhas com Palavra-chave (`-l`)
- Conta o número de linhas num documento que contêm uma palavra-chave.
- Implementado através de `fork` e `exec` com o comando `grep -c`.

### 🛑 Encerramento do Servidor (`-f`)
- Encerra de forma segura o servidor, garantindo a escrita dos dados persistentes em `meta_data.txt`.

---

## 🛠️ Estrutura do Projeto

O projeto está organizado de forma modular e cumpre todos os requisitos básicos de programação:

📁 `src/` — Código-fonte:
- `dserver.c` — Implementação do servidor.
- `dclient.c` — Implementação do cliente.
- `index.c` — Gestão do índice de documentos.
- `common.h` — Definições comuns (estruturas, constantes, enums).
- `server.h` / `client.h` / `index.h` — Headers específicos por módulo.

📁 `include/` — Headers para modularização.

📁 `bin/` — Executáveis compilados:
- `dserver`
- `dclient`

📁 `docs/` — Documentos a indexar (ficheiros `.txt`).

📁 `tmp/` — Diretório auxiliar para uso interno.

📄 `meta_data.txt` — Ficheiro persistente com os metadados dos documentos.

📄 `Makefile` — Compilação automática de todos os ficheiros.

---

## 🚀 Como Executar

### 📦 Compilar o Projeto
```bash
make
```

### ▶️ Executar o Servidor
```bash
./bin/dserver docs 10
```
- O `10` representa o número máximo de documentos a manter em cache (não implementado ainda).

### 🧑‍💻 Executar o Cliente

#### Adicionar documento:
```bash
./bin/dclient -a "Romeo and Juliet" "William Shakespeare" "1997" "docs/1112.txt"
```

#### Consultar documento:
```bash
./bin/dclient -c 1
```

#### Contar linhas com palavra-chave:
```bash
./bin/dclient -l 1 "Romeo"
```

#### Encerrar servidor:
```bash
./bin/dclient -f
```

---

## 📈 Estado Atual do Projeto

| Comando | Estado | Observações |
|---------|--------|-------------|
| `-a`    | ✅     | Adição de documentos funcional |
| `-c`    | ✅     | Consulta de metadados funcional |
| `-l`    | ✅     | Contagem com `grep` funcional |
| `-f`    | ✅     | Encerra o servidor com persistência |
| `-d`    | ❌     | A remover documento será a próxima tarefa |
| `-s`    | ❌     | Pesquisa com múltiplos processos ainda por implementar |
| `Cache` | ❌     | Ainda não foi implementada a lógica de cache |

---

## 📜 Autores

Este projeto foi desenvolvido por:

- Hélder Tiago Peixoto da Cruz - A104174  
- André Miguel Rego Trindade Pinto - A104267
- Rafael Airosa Pereira - A...
