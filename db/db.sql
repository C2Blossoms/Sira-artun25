--
-- PostgreSQL database cluster dump
--

-- Started on 2025-09-19 01:45:36

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








--
-- Databases
--

--
-- Database "template1" dump
--

\connect template1

--
-- PostgreSQL database dump
--

-- Dumped from database version 17.5
-- Dumped by pg_dump version 17.5

-- Started on 2025-09-19 01:45:36

SET statement_timeout = 0;
SET lock_timeout = 0;
SET idle_in_transaction_session_timeout = 0;
SET transaction_timeout = 0;
SET client_encoding = 'UTF8';
SET standard_conforming_strings = on;
SELECT pg_catalog.set_config('search_path', '', false);
SET check_function_bodies = false;
SET xmloption = content;
SET client_min_messages = warning;
SET row_security = off;

-- Completed on 2025-09-19 01:45:36

--
-- PostgreSQL database dump complete
--

--
-- Database "postgres" dump
--

\connect postgres

--
-- PostgreSQL database dump
--

-- Dumped from database version 17.5
-- Dumped by pg_dump version 17.5

-- Started on 2025-09-19 01:45:36

SET statement_timeout = 0;
SET lock_timeout = 0;
SET idle_in_transaction_session_timeout = 0;
SET transaction_timeout = 0;
SET client_encoding = 'UTF8';
SET standard_conforming_strings = on;
SELECT pg_catalog.set_config('search_path', '', false);
SET check_function_bodies = false;
SET xmloption = content;
SET client_min_messages = warning;
SET row_security = off;

SET default_tablespace = '';

SET default_table_access_method = heap;

--
-- TOC entry 222 (class 1259 OID 16482)
-- Name: cards; Type: TABLE; Schema: public; Owner: postgres
--

CREATE TABLE public.cards (
    card_id integer NOT NULL,
    student_id integer,
    balance numeric(10,2)
);


ALTER TABLE public.cards OWNER TO postgres;

--
-- TOC entry 221 (class 1259 OID 16481)
-- Name: cards_card_id_seq; Type: SEQUENCE; Schema: public; Owner: postgres
--

CREATE SEQUENCE public.cards_card_id_seq
    AS integer
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER SEQUENCE public.cards_card_id_seq OWNER TO postgres;

--
-- TOC entry 4931 (class 0 OID 0)
-- Dependencies: 221
-- Name: cards_card_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: postgres
--

ALTER SEQUENCE public.cards_card_id_seq OWNED BY public.cards.card_id;


--
-- TOC entry 218 (class 1259 OID 16468)
-- Name: students; Type: TABLE; Schema: public; Owner: postgres
--

CREATE TABLE public.students (
    stdent_id integer NOT NULL,
    full_name character varying(100) NOT NULL,
    class character varying(100)
);


ALTER TABLE public.students OWNER TO postgres;

--
-- TOC entry 217 (class 1259 OID 16467)
-- Name: students_stdent_id_seq; Type: SEQUENCE; Schema: public; Owner: postgres
--

CREATE SEQUENCE public.students_stdent_id_seq
    AS integer
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER SEQUENCE public.students_stdent_id_seq OWNER TO postgres;

--
-- TOC entry 4932 (class 0 OID 0)
-- Dependencies: 217
-- Name: students_stdent_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: postgres
--

ALTER SEQUENCE public.students_stdent_id_seq OWNED BY public.students.stdent_id;


--
-- TOC entry 224 (class 1259 OID 16495)
-- Name: transaction_logs; Type: TABLE; Schema: public; Owner: postgres
--

CREATE TABLE public.transaction_logs (
    txn_id integer NOT NULL,
    card_id integer,
    vendor_id integer,
    amount numeric(10,2) NOT NULL,
    purpose character varying(50),
    txn_time timestamp without time zone DEFAULT CURRENT_TIMESTAMP,
    stripe_payment_id character varying
);


ALTER TABLE public.transaction_logs OWNER TO postgres;

--
-- TOC entry 223 (class 1259 OID 16494)
-- Name: transaction_logs_txn_id_seq; Type: SEQUENCE; Schema: public; Owner: postgres
--

CREATE SEQUENCE public.transaction_logs_txn_id_seq
    AS integer
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER SEQUENCE public.transaction_logs_txn_id_seq OWNER TO postgres;

--
-- TOC entry 4933 (class 0 OID 0)
-- Dependencies: 223
-- Name: transaction_logs_txn_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: postgres
--

ALTER SEQUENCE public.transaction_logs_txn_id_seq OWNED BY public.transaction_logs.txn_id;


--
-- TOC entry 220 (class 1259 OID 16475)
-- Name: vendors; Type: TABLE; Schema: public; Owner: postgres
--

CREATE TABLE public.vendors (
    vendor_id integer NOT NULL,
    shop_name character varying(100) NOT NULL,
    owner_name character varying(100)
);


ALTER TABLE public.vendors OWNER TO postgres;

--
-- TOC entry 219 (class 1259 OID 16474)
-- Name: vendors_vendor_id_seq; Type: SEQUENCE; Schema: public; Owner: postgres
--

CREATE SEQUENCE public.vendors_vendor_id_seq
    AS integer
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER SEQUENCE public.vendors_vendor_id_seq OWNER TO postgres;

--
-- TOC entry 4934 (class 0 OID 0)
-- Dependencies: 219
-- Name: vendors_vendor_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: postgres
--

ALTER SEQUENCE public.vendors_vendor_id_seq OWNED BY public.vendors.vendor_id;


--
-- TOC entry 4759 (class 2604 OID 16485)
-- Name: cards card_id; Type: DEFAULT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.cards ALTER COLUMN card_id SET DEFAULT nextval('public.cards_card_id_seq'::regclass);


--
-- TOC entry 4757 (class 2604 OID 16471)
-- Name: students stdent_id; Type: DEFAULT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.students ALTER COLUMN stdent_id SET DEFAULT nextval('public.students_stdent_id_seq'::regclass);


--
-- TOC entry 4760 (class 2604 OID 16498)
-- Name: transaction_logs txn_id; Type: DEFAULT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.transaction_logs ALTER COLUMN txn_id SET DEFAULT nextval('public.transaction_logs_txn_id_seq'::regclass);


--
-- TOC entry 4758 (class 2604 OID 16478)
-- Name: vendors vendor_id; Type: DEFAULT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.vendors ALTER COLUMN vendor_id SET DEFAULT nextval('public.vendors_vendor_id_seq'::regclass);


--
-- TOC entry 4923 (class 0 OID 16482)
-- Dependencies: 222
-- Data for Name: cards; Type: TABLE DATA; Schema: public; Owner: postgres
--

COPY public.cards (card_id, student_id, balance) FROM stdin;
1	66200001	668.00
2	66200002	850.00
\.


--
-- TOC entry 4919 (class 0 OID 16468)
-- Dependencies: 218
-- Data for Name: students; Type: TABLE DATA; Schema: public; Owner: postgres
--

COPY public.students (stdent_id, full_name, class) FROM stdin;
66200001	bob	3
66200002	Tom	3
\.


--
-- TOC entry 4925 (class 0 OID 16495)
-- Dependencies: 224
-- Data for Name: transaction_logs; Type: TABLE DATA; Schema: public; Owner: postgres
--

COPY public.transaction_logs (txn_id, card_id, vendor_id, amount, purpose, txn_time, stripe_payment_id) FROM stdin;
2	1	\N	50.00	topup	2025-08-17 16:15:21.025651	\N
4	1	\N	50.00	topup	2025-08-17 16:18:01.873144	\N
6	1	\N	50.00	topup	2025-08-17 16:20:46.211211	\N
8	1	\N	50.00	topup	2025-08-17 16:33:34.072413	\N
9	1	1	50.00	payment	2025-08-17 16:33:53.628892	\N
10	1	\N	50.00	topup	2025-08-17 16:34:51.077074	\N
11	1	1	50.00	payment	2025-08-17 16:34:55.134735	\N
12	1	2	50.00	payment	2025-08-17 16:34:58.430098	\N
13	1	\N	100.00	topup	2025-08-17 16:35:14.869287	\N
14	1	\N	100.00	topup	2025-08-17 23:19:35.730839	\N
15	1	2	50.00	payment	2025-08-18 00:20:22.69383	\N
16	1	\N	100.00	topup	2025-08-18 03:42:37.779676	\N
17	1	\N	100.00	topup	2025-08-18 03:44:35.845597	\N
18	1	\N	1.00	topup	2025-08-18 03:46:56.993058	\N
19	1	2	50.00	payment	2025-08-18 03:52:40.863864	\N
20	1	1	50.00	payment	2025-08-18 03:52:47.577072	\N
21	1	2	50.00	payment	2025-08-18 03:52:53.924042	\N
22	1	2	50.00	payment	2025-08-18 10:12:22.366858	\N
23	1	\N	1.00	topup	2025-08-18 10:23:35.121666	\N
24	1	\N	1.00	topup	2025-08-18 11:55:05.605031	\N
25	1	\N	1.00	topup	2025-09-18 22:48:11.811729	\N
26	1	\N	1.00	topup	2025-09-18 22:48:15.389104	\N
27	1	\N	1.00	topup	2025-09-18 22:48:16.998054	\N
28	1	\N	1.00	topup	2025-09-18 22:48:20.083319	\N
29	1	\N	1.00	topup	2025-09-18 22:48:20.985751	\N
30	1	\N	1.00	topup	2025-09-18 22:48:21.757912	\N
31	1	\N	1.00	topup	2025-09-18 22:48:22.427237	\N
32	1	2	50.00	payment	2025-09-18 22:48:27.401846	\N
33	1	2	50.00	payment	2025-09-18 22:48:28.506403	\N
34	1	2	50.00	payment	2025-09-18 22:48:29.325084	\N
35	1	1	50.00	payment	2025-09-18 22:49:36.391398	\N
36	1	1	50.00	payment	2025-09-18 22:49:38.521094	\N
37	1	1	50.00	payment	2025-09-18 22:49:39.968537	\N
38	1	1	50.00	payment	2025-09-18 22:49:41.097749	\N
39	1	1	50.00	payment	2025-09-18 22:49:41.947809	\N
40	1	2	50.00	payment	2025-09-18 22:49:44.893189	\N
42	1	2	50.00	payment	2025-09-18 22:51:20.27285	\N
43	1	\N	1.00	topup	2025-09-18 22:51:25.36278	\N
44	1	\N	1.00	topup	2025-09-18 22:51:26.398318	\N
45	1	\N	1.00	topup	2025-09-18 22:51:27.145827	\N
46	1	\N	50.00	topup	2025-09-18 22:51:31.477316	\N
47	1	\N	50.00	topup	2025-09-18 22:51:32.811166	\N
48	1	\N	50.00	topup	2025-09-18 22:51:35.402383	\N
49	1	\N	50.00	topup	2025-09-18 22:51:36.099116	\N
50	1	\N	50.00	topup	2025-09-18 22:51:36.633552	\N
51	1	\N	50.00	topup	2025-09-18 22:51:37.262847	\N
52	1	\N	50.00	topup	2025-09-18 23:18:05.440526	\N
53	2	\N	50.00	topup	2025-09-18 23:20:29.210032	\N
54	2	\N	50.00	topup	2025-09-18 23:20:34.4364	\N
55	2	\N	50.00	topup	2025-09-18 23:20:35.979495	\N
56	2	\N	50.00	topup	2025-09-18 23:20:38.230618	\N
57	2	2	50.00	payment	2025-09-18 23:20:45.638929	\N
58	2	2	50.00	payment	2025-09-18 23:20:46.93183	\N
59	2	\N	500.00	topup	2025-09-19 00:03:19.821166	\N
60	2	\N	500.00	topup	2025-09-19 00:03:21.795077	\N
61	2	2	50.00	payment	2025-09-19 00:03:27.06923	\N
62	2	2	50.00	payment	2025-09-19 00:03:27.933322	\N
63	2	2	50.00	payment	2025-09-19 00:03:28.879356	\N
64	2	2	50.00	payment	2025-09-19 00:03:29.535141	\N
65	2	2	50.00	payment	2025-09-19 00:03:30.295739	\N
\.


--
-- TOC entry 4921 (class 0 OID 16475)
-- Dependencies: 220
-- Data for Name: vendors; Type: TABLE DATA; Schema: public; Owner: postgres
--

COPY public.vendors (vendor_id, shop_name, owner_name) FROM stdin;
1	noodle_shop	tong
2	Stack_shop	fang
\.


--
-- TOC entry 4935 (class 0 OID 0)
-- Dependencies: 221
-- Name: cards_card_id_seq; Type: SEQUENCE SET; Schema: public; Owner: postgres
--

SELECT pg_catalog.setval('public.cards_card_id_seq', 1, false);


--
-- TOC entry 4936 (class 0 OID 0)
-- Dependencies: 217
-- Name: students_stdent_id_seq; Type: SEQUENCE SET; Schema: public; Owner: postgres
--

SELECT pg_catalog.setval('public.students_stdent_id_seq', 1, false);


--
-- TOC entry 4937 (class 0 OID 0)
-- Dependencies: 223
-- Name: transaction_logs_txn_id_seq; Type: SEQUENCE SET; Schema: public; Owner: postgres
--

SELECT pg_catalog.setval('public.transaction_logs_txn_id_seq', 65, true);


--
-- TOC entry 4938 (class 0 OID 0)
-- Dependencies: 219
-- Name: vendors_vendor_id_seq; Type: SEQUENCE SET; Schema: public; Owner: postgres
--

SELECT pg_catalog.setval('public.vendors_vendor_id_seq', 1, false);


--
-- TOC entry 4767 (class 2606 OID 16487)
-- Name: cards cards_pkey; Type: CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.cards
    ADD CONSTRAINT cards_pkey PRIMARY KEY (card_id);


--
-- TOC entry 4763 (class 2606 OID 16473)
-- Name: students students_pkey; Type: CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.students
    ADD CONSTRAINT students_pkey PRIMARY KEY (stdent_id);


--
-- TOC entry 4769 (class 2606 OID 16501)
-- Name: transaction_logs transaction_logs_pkey; Type: CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.transaction_logs
    ADD CONSTRAINT transaction_logs_pkey PRIMARY KEY (txn_id);


--
-- TOC entry 4765 (class 2606 OID 16480)
-- Name: vendors vendors_pkey; Type: CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.vendors
    ADD CONSTRAINT vendors_pkey PRIMARY KEY (vendor_id);


--
-- TOC entry 4771 (class 2606 OID 16502)
-- Name: transaction_logs fk_card_id; Type: FK CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.transaction_logs
    ADD CONSTRAINT fk_card_id FOREIGN KEY (card_id) REFERENCES public.cards(card_id);


--
-- TOC entry 4772 (class 2606 OID 16507)
-- Name: transaction_logs fk_vendor_id; Type: FK CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.transaction_logs
    ADD CONSTRAINT fk_vendor_id FOREIGN KEY (vendor_id) REFERENCES public.vendors(vendor_id);


--
-- TOC entry 4770 (class 2606 OID 16488)
-- Name: cards student_id; Type: FK CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.cards
    ADD CONSTRAINT student_id FOREIGN KEY (student_id) REFERENCES public.students(stdent_id);


-- Completed on 2025-09-19 01:45:36

--
-- PostgreSQL database dump complete
--

-- Completed on 2025-09-19 01:45:36

--
-- PostgreSQL database cluster dump complete
--

