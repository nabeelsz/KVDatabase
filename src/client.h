/*
    Client.h
    The client file is used for the client-side interface, allowing clients to send requests to the database and the database
    being able to process them, and then respond to the client. The client-side interface is multithreaded, so that instead of 
    only processing one request a time, multiple can be processed at the same time. 
*/