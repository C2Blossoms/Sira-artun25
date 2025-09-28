--
-- PostgreSQL database cluster dump
--

-- Started on 2025-09-28 21:50:28

SET default_transaction_read_only = off;

SET client_encoding = 'UTF8';
SET standard_conforming_strings = on;

--
-- Roles
--

CREATE ROLE postgres;
ALTER ROLE postgres WITH SUPERUSER INHERIT CREATEROLE CREATEDB LOGIN REPLICATION BYPASSRLS PASSWORD 'SCRAM-SHA-256$4096:UnJOwKzhjnewGa3KLilscA==$xRJjfc6yAQCVd0Mg3AAtM3sJF6dHEXKZxnhx/vHi568=:4htzgMch4PBk4aWSRKNGL9zXgQ1dRvseboKgk8n/E0o=';

--
-- User Configurations
--








-- Completed on 2025-09-28 21:50:28

--
-- PostgreSQL database cluster dump complete
--

