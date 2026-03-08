g++ -std=c++17 -Iinclude -I. \
main.cpp \
auth/auth.cpp \
utils/dotenv.cpp \
clients/kicksdb_client.cpp \
database/dbclient.cpp \
-o main \
-lssl -lcrypto -pthread