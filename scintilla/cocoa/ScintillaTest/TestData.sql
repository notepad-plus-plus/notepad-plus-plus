-- MySQL Administrator dump 1.4
--
-- ------------------------------------------------------
-- Server version	5.0.45


/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!40101 SET NAMES utf8 */;

/*!40014 SET @OLD_UNIQUE_CHECKS=@@UNIQUE_CHECKS, UNIQUE_CHECKS=0 */;
/*!40014 SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0 */;
/*!40101 SET @OLD_SQL_MODE=@@SQL_MODE, SQL_MODE='NO_AUTO_VALUE_ON_ZERO,ANSI_QUOTES' */;

/**
 * Foldable multiline comment.
 */

-- {
-- Create schema sakila
-- }

CREATE DATABASE IF NOT EXISTS sakila;
USE sakila;
DROP TABLE IF EXISTS "sakila"."actor_info";
DROP VIEW IF EXISTS "sakila"."actor_info";
CREATE TABLE "sakila"."actor_info" (
  "actor_id" smallint(5) unsigned,
  "first_name" varchar(45),
  "last_name" varchar(45),
  "film_info" varchar(341)
);
DROP TABLE IF EXISTS "sakila"."actor";
CREATE TABLE  "sakila"."actor" (
  "actor_id" smallint(5) unsigned NOT NULL auto_increment,
  "first_name" varchar(45) NOT NULL,
  "last_name" varchar(45) NOT NULL,
  "last_update" timestamp NOT NULL default CURRENT_TIMESTAMP on update CURRENT_TIMESTAMP,
  PRIMARY KEY  ("actor_id"),
  KEY "idx_actor_last_name" ("last_name")
) ENGINE=InnoDB AUTO_INCREMENT=201 DEFAULT CHARSET=utf8;
INSERT INTO "sakila"."actor" VALUES  (1,'PENELOPE','GUINESS','2006-02-15 04:34:33'),
 (2,'NICK','WAHLBERG','2006-02-15 04:34:33'),
 (3,'ED','CHASE','2006-02-15 04:34:33'),
 (4,'JENNIFER','DAVIS','2006-02-15 04:34:33'),
 (149,'RUSSELL','TEMPLE','2006-02-15 04:34:33'),
 (150,'JAYNE','NOLTE','2006-02-15 04:34:33'),
 (151,'GEOFFREY','HESTON','2006-02-15 04:34:33'),
 (152,'BEN','HARRIS','2006-02-15 04:34:33'),
 (153,'MINNIE','KILMER','2006-02-15 04:34:33'),
 (154,'MERYL','GIBSON','2006-02-15 04:34:33'),
 (155,'IAN','TANDY','2006-02-15 04:34:33'),
 (156,'FAY','WOOD','2006-02-15 04:34:33'),
 (157,'GRETA','MALDEN','2006-02-15 04:34:33'),
 (158,'VIVIEN','BASINGER','2006-02-15 04:34:33'),
 (159,'LAURA','BRODY','2006-02-15 04:34:33'),
 (160,'CHRIS','DEPP','2006-02-15 04:34:33'),
 (161,'HARVEY','HOPE','2006-02-15 04:34:33'),
 (162,'OPRAH','KILMER','2006-02-15 04:34:33'),
 (163,'CHRISTOPHER','WEST','2006-02-15 04:34:33'),
 (164,'HUMPHREY','WILLIS','2006-02-15 04:34:33'),
 (165,'AL','GARLAND','2006-02-15 04:34:33'),
 (166,'NICK','DEGENERES','2006-02-15 04:34:33'),
 (167,'LAURENCE','BULLOCK','2006-02-15 04:34:33'),
 (168,'WILL','WILSON','2006-02-15 04:34:33'),
 (169,'KENNETH','HOFFMAN','2006-02-15 04:34:33'),
 (170,'MENA','HOPPER','2006-02-15 04:34:33'),
 (171,'OLYMPIA','PFEIFFER','2006-02-15 04:34:33'),
 (190,'AUDREY','BAILEY','2006-02-15 04:34:33'),
 (191,'GREGORY','GOODING','2006-02-15 04:34:33'),
 (192,'JOHN','SUVARI','2006-02-15 04:34:33'),
 (193,'BURT','TEMPLE','2006-02-15 04:34:33'),
 (194,'MERYL','ALLEN','2006-02-15 04:34:33'),
 (195,'JAYNE','SILVERSTONE','2006-02-15 04:34:33'),
 (196,'BELA','WALKEN','2006-02-15 04:34:33'),
 (197,'REESE','WEST','2006-02-15 04:34:33'),
 (198,'MARY','KEITEL','2006-02-15 04:34:33'),
 (199,'JULIA','FAWCETT','2006-02-15 04:34:33'),
 (200,'THORA','TEMPLE','2006-02-15 04:34:33');

DROP TRIGGER /*!50030 IF EXISTS */ "sakila"."payment_date";

DELIMITER $$

CREATE DEFINER = "root"@"localhost" TRIGGER  "sakila"."payment_date" BEFORE INSERT ON "payment" FOR EACH ROW SET NEW.payment_date = NOW() $$

DELIMITER ;


DROP TABLE IF EXISTS "sakila"."sales_by_store";
DROP VIEW IF EXISTS "sakila"."sales_by_store";
CREATE ALGORITHM=UNDEFINED DEFINER="root"@"localhost" SQL SECURITY DEFINER VIEW "sakila"."sales_by_store" AS select concat("c"."city",_utf8',',"cy"."country") AS "store",concat("m"."first_name",_utf8' ',"m"."last_name") AS "manager",sum("p"."amount") AS "total_sales" from ((((((("sakila"."payment" "p" join "sakila"."rental" "r" on(("p"."rental_id" = "r"."rental_id"))) join "sakila"."inventory" "i" on(("r"."inventory_id" = "i"."inventory_id"))) join "sakila"."store" "s" on(("i"."store_id" = "s"."store_id"))) join "sakila"."address" "a" on(("s"."address_id" = "a"."address_id"))) join "sakila"."city" "c" on(("a"."city_id" = "c"."city_id"))) join "sakila"."country" "cy" on(("c"."country_id" = "cy"."country_id"))) join "sakila"."staff" "m" on(("s"."manager_staff_id" = "m"."staff_id"))) group by "s"."store_id" order by "cy"."country","c"."city";

--
-- View structure for view `staff_list`
--

CREATE VIEW staff_list 
AS 
SELECT s.staff_id AS ID, CONCAT(s.first_name, _utf8' ', s.last_name) AS name, a.address AS address, a.postal_code AS `zip code`, a.phone AS phone,
	city.city AS city, country.country AS country, s.store_id AS SID 
FROM staff AS s JOIN address AS a ON s.address_id = a.address_id JOIN city ON a.city_id = city.city_id 
	JOIN country ON city.country_id = country.country_id;

--
-- View structure for view `actor_info`
--

CREATE DEFINER=CURRENT_USER SQL SECURITY INVOKER VIEW actor_info 
AS
SELECT      
a.actor_id,
a.first_name,
a.last_name,
GROUP_CONCAT(DISTINCT CONCAT(c.name, ': ',
		(SELECT GROUP_CONCAT(f.title ORDER BY f.title SEPARATOR ', ')
                    FROM sakila.film f
                    INNER JOIN sakila.film_category fc
                      ON f.film_id = fc.film_id
                    INNER JOIN sakila.film_actor fa
                      ON f.film_id = fa.film_id
                    WHERE fc.category_id = c.category_id
                    AND fa.actor_id = a.actor_id
                 )
             )
             ORDER BY c.name SEPARATOR '; ')
AS film_info
FROM sakila.actor a
LEFT JOIN sakila.film_actor fa
  ON a.actor_id = fa.actor_id
LEFT JOIN sakila.film_category fc
  ON fa.film_id = fc.film_id
LEFT JOIN sakila.category c
  ON fc.category_id = c.category_id
GROUP BY a.actor_id, a.first_name, a.last_name;

DELIMITER $$

CREATE FUNCTION get_customer_balance(p_customer_id INT, p_effective_date DATETIME) RETURNS DECIMAL(5,2)
    DETERMINISTIC
    READS SQL DATA
BEGIN

       #OK, WE NEED TO CALCULATE THE CURRENT BALANCE GIVEN A CUSTOMER_ID AND A DATE
       #THAT WE WANT THE BALANCE TO BE EFFECTIVE FOR. THE BALANCE IS:
       #   1) RENTAL FEES FOR ALL PREVIOUS RENTALS
       #   2) ONE DOLLAR FOR EVERY DAY THE PREVIOUS RENTALS ARE OVERDUE
       #   3) IF A FILM IS MORE THAN RENTAL_DURATION * 2 OVERDUE, CHARGE THE REPLACEMENT_COST
       #   4) SUBTRACT ALL PAYMENTS MADE BEFORE THE DATE SPECIFIED

  DECLARE v_rentfees DECIMAL(5,2); #FEES PAID TO RENT THE VIDEOS INITIALLY
  DECLARE v_overfees INTEGER;      #LATE FEES FOR PRIOR RENTALS
  DECLARE v_payments DECIMAL(5,2); #SUM OF PAYMENTS MADE PREVIOUSLY

  SELECT IFNULL(SUM(film.rental_rate),0) INTO v_rentfees
    FROM film, inventory, rental
    WHERE film.film_id = inventory.film_id
      AND inventory.inventory_id = rental.inventory_id
      AND rental.rental_date <= p_effective_date
      AND rental.customer_id = p_customer_id;

  SELECT IFNULL(SUM(IF((TO_DAYS(rental.return_date) - TO_DAYS(rental.rental_date)) > film.rental_duration,
        ((TO_DAYS(rental.return_date) - TO_DAYS(rental.rental_date)) - film.rental_duration),0)),0) INTO v_overfees
    FROM rental, inventory, film
    WHERE film.film_id = inventory.film_id
      AND inventory.inventory_id = rental.inventory_id
      AND rental.rental_date <= p_effective_date
      AND rental.customer_id = p_customer_id;


  SELECT IFNULL(SUM(payment.amount),0) INTO v_payments
    FROM payment

    WHERE payment.payment_date <= p_effective_date
    AND payment.customer_id = p_customer_id;

  RETURN v_rentfees + v_overfees - v_payments;
END $$

DELIMITER ;

DELIMITER $$

CREATE FUNCTION inventory_in_stock(p_inventory_id INT) RETURNS BOOLEAN
READS SQL DATA
BEGIN
    DECLARE v_rentals INT;
    DECLARE v_out     INT;

    #AN ITEM IS IN-STOCK IF THERE ARE EITHER NO ROWS IN THE rental TABLE
    #FOR THE ITEM OR ALL ROWS HAVE return_date POPULATED

    SELECT COUNT(*) INTO v_rentals
    FROM rental
    WHERE inventory_id = p_inventory_id;

    IF v_rentals = 0 THEN
      RETURN TRUE;
    END IF;

    SELECT COUNT(rental_id) INTO v_out
    FROM inventory LEFT JOIN rental USING(inventory_id)
    WHERE inventory.inventory_id = p_inventory_id
    AND rental.return_date IS NULL;

    IF v_out > 0 THEN
      RETURN FALSE;
    ELSE
      RETURN TRUE;
    END IF;
END $$

DELIMITER ;

