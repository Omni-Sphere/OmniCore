-- =============================================================================
-- OmniCore.sql - Master Platform Schema DDL & Data Migration Script
-- Strict PostgreSQL Case Preservation with Double Quotes
-- Modules: Identity, Users, Sessions, GlobalConfiguration, SystemConfig,
--          Venues, Events, DeparturePoints, Routes, RouteStops, Schedules,
--          Tickets, NotificationContacts, NotificationSettings, Reservations,
--          Multi-Gateway Payments (Stripe, OpenPay, MercadoPago),
--          WhatsApp Cloud API & Custom Notification Templates
-- =============================================================================

-- 1. Custom ENUM Types Check
DO $$ 
BEGIN
    IF NOT EXISTS (SELECT 1 FROM pg_type WHERE typname = 'PermissionMode') THEN
        CREATE TYPE "PermissionMode" AS ENUM ('P', 'R');
    END IF;

    IF NOT EXISTS (SELECT 1 FROM pg_type WHERE typname = 'PaymentMethodType') THEN
        CREATE TYPE "PaymentMethodType" AS ENUM (
            'NOT_APPLICABLE',
            'CASH',
            'TRANSFER',
            'CARD',
            'OTHER'
        );
    END IF;

    IF NOT EXISTS (SELECT 1 FROM pg_type WHERE typname = 'PaymentStatusType') THEN
        CREATE TYPE "PaymentStatusType" AS ENUM (
            'UNPAID',
            'PENDING',
            'PAID',
            'REFUNDED',
            'CANCELLED'
        );
    END IF;

    IF NOT EXISTS (SELECT 1 FROM pg_type WHERE typname = 'DeparturePointType') THEN
        CREATE TYPE "DeparturePointType" AS ENUM (
            'PICKUP',
            'INTERMEDIATE',
            'DROPOFF'
        );
    END IF;

    IF NOT EXISTS (SELECT 1 FROM pg_type WHERE typname = 'ScheduleType') THEN
        CREATE TYPE "ScheduleType" AS ENUM (
            'DEPARTURE',
            'RETURN',
            'ROUND_TRIP'
        );
    END IF;

    IF NOT EXISTS (SELECT 1 FROM pg_type WHERE typname = 'EventCategory') THEN
        CREATE TYPE "EventCategory" AS ENUM (
            'MUSIC',
            'SPORTS',
            'THEATER',
            'FESTIVAL',
            'CULTURE',
            'OTHER'
        );
    END IF;

    IF NOT EXISTS (SELECT 1 FROM pg_type WHERE typname = 'ReservationStatusType') THEN
        CREATE TYPE "ReservationStatusType" AS ENUM (
            'PENDING',
            'UNCONFIRMED',
            'CONFIRMED',
            'CANCELLED',
            'COMPLETED'
        );
    END IF;

    -- Migration: add PENDING to ReservationStatusType if missing
    IF EXISTS (SELECT 1 FROM pg_type WHERE typname = 'ReservationStatusType') THEN
        IF NOT EXISTS (SELECT 1 FROM pg_enum WHERE enumlabel = 'PENDING' AND enumtypid = (SELECT oid FROM pg_type WHERE typname = 'ReservationStatusType')) THEN
            ALTER TYPE "ReservationStatusType" ADD VALUE IF NOT EXISTS 'PENDING' BEFORE 'UNCONFIRMED';
        END IF;
    END IF;
END $$;

-- 2. Users
CREATE TABLE IF NOT EXISTS "Users" (
    "Entry" SERIAL PRIMARY KEY,
    "Code" VARCHAR(50) NOT NULL UNIQUE,
    "Name" VARCHAR(100) NOT NULL UNIQUE,
    "Email" VARCHAR(50),
    "Phone" VARCHAR(50),
    "Employee" INT,
    "Department" INT,
    "PermissionMode" "PermissionMode" NOT NULL DEFAULT 'P',
    "RoleEntry" INT,
    "MaxDisccountPerLine" NUMERIC(12, 6),
    "MaxDisccountPerDocument" NUMERIC(12, 6),
    "SuperUser" BOOLEAN NOT NULL DEFAULT false,
    "IsLocked" BOOLEAN NOT NULL DEFAULT false,
    "IsActive" BOOLEAN NOT NULL DEFAULT true,
    "Password" BYTEA,
    "ChangePasswordNextLogin" BOOLEAN NOT NULL DEFAULT false,
    "PasswordNeverExpires" BOOLEAN NOT NULL DEFAULT false,
    "CreatedBy" VARCHAR(20) NOT NULL DEFAULT 'system',
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "LastUpdatedBy" VARCHAR(20),
    "UpdateDate" TIMESTAMP,
    "EmployeeCode" VARCHAR(50)
);

-- Migration for existing Users table
ALTER TABLE "Users" ADD COLUMN IF NOT EXISTS "EmployeeCode" VARCHAR(50);
CREATE INDEX IF NOT EXISTS "IDX_Users_EmployeeCode" ON "Users" ("EmployeeCode");

-- 2.1 Employees (Agnostic Master Entity)
CREATE TABLE IF NOT EXISTS "Employees" (
    "Entry" SERIAL PRIMARY KEY,
    "Code" VARCHAR(50) NOT NULL UNIQUE,
    "Name" VARCHAR(255) NOT NULL,
    "FirstName" VARCHAR(100),
    "SecondName" VARCHAR(100),
    "LastName" VARCHAR(100),
    "SecondLastName" VARCHAR(100),
    "Email" VARCHAR(100),
    "Phone" VARCHAR(50),
    "Department" VARCHAR(100),
    "Position" VARCHAR(100),
    "DirectManagerCode" VARCHAR(50),
    "DateOfBirth" DATE,
    "Comments" TEXT,
    "IsActive" BOOLEAN NOT NULL DEFAULT true,
    "IsCanceled" BOOLEAN NOT NULL DEFAULT false,
    "CreatedBy" VARCHAR(20) NOT NULL DEFAULT 'system',
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "LastUpdatedBy" VARCHAR(20),
    "UpdateDate" TIMESTAMP
);
CREATE INDEX IF NOT EXISTS "IDX_Employees_Code_Active" ON "Employees" ("Code") WHERE "IsCanceled" = false;
CREATE INDEX IF NOT EXISTS "IDX_Employees_Email" ON "Employees" ("Email");

-- Users.EmployeeCode is the single source of truth for employee-user links.
-- Preserve links from installations that previously stored them on Employees.
DO $$
BEGIN
    IF EXISTS (
        SELECT 1 FROM information_schema.columns
        WHERE table_name = 'Employees' AND column_name = 'UserCode'
    ) THEN
        UPDATE "Users" AS u
        SET "EmployeeCode" = e."Code"
        FROM "Employees" AS e
        WHERE e."UserCode" = u."Code"
          AND (u."EmployeeCode" IS NULL OR u."EmployeeCode" = '');

        ALTER TABLE "Employees" DROP COLUMN "UserCode";
    END IF;
END $$;

-- 3. Sessions
CREATE TABLE IF NOT EXISTS "Sessions" (
    "SessionEntry" SERIAL PRIMARY KEY,
    "SessionUUID" VARCHAR(100) NOT NULL UNIQUE DEFAULT gen_random_uuid()::text,
    "UserCode" VARCHAR(50),
    "UserEmail" VARCHAR(50),
    "UserPhone" VARCHAR(50),
    "StartDate" VARCHAR(50),
    "EndDate" VARCHAR(50),
    "DurationSeconds" INT DEFAULT 0,
    "DeviceIP" VARCHAR(50),
    "HostName" VARCHAR(100),
    "IsActive" CHAR(1) NOT NULL DEFAULT 'Y',
    "Reason" VARCHAR(255),
    "LogoutMessage" VARCHAR(255),
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP
);
CREATE INDEX IF NOT EXISTS "IDX_Sessions_SessionUUID" ON "Sessions" ("SessionUUID");
CREATE INDEX IF NOT EXISTS "IDX_Sessions_UserCode_IsActive" ON "Sessions" ("UserCode", "IsActive");

-- 4. GlobalConfiguration
CREATE TABLE IF NOT EXISTS "GlobalConfiguration" (
    "Entry" SERIAL PRIMARY KEY,
    "Code" VARCHAR(50) NOT NULL UNIQUE DEFAULT 'DEFAULT',
    "Name" VARCHAR(100) NOT NULL,
    "Value" TEXT,
    "IsEncrypted" BOOLEAN NOT NULL DEFAULT false,
    "IsActive" BOOLEAN NOT NULL DEFAULT true,
    "CreatedBy" VARCHAR(20) NOT NULL DEFAULT 'system',
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "LastUpdatedBy" VARCHAR(20),
    "UpdateDate" TIMESTAMP
);

-- 5. SystemConfigs
CREATE TABLE IF NOT EXISTS "SystemConfigs" (
    "Entry" SERIAL PRIMARY KEY,
    "Code" VARCHAR(50) NOT NULL DEFAULT 'DEFAULT' UNIQUE,
    "FeeHandlingStrategy" VARCHAR(50) NOT NULL DEFAULT 'SURCHARGE',
    "TaxRatePercent" NUMERIC(5, 2) NOT NULL DEFAULT 16.00,
    "DefaultCurrency" VARCHAR(10) NOT NULL DEFAULT 'MXN',
    "CompanyName" VARCHAR(255) NOT NULL DEFAULT 'OmniSphere Enterprise',
    "EnableEmailNotifications" BOOLEAN NOT NULL DEFAULT true,
    "EnableWhatsappNotifications" BOOLEAN NOT NULL DEFAULT true,
    "AllowPartialPayments" BOOLEAN NOT NULL DEFAULT false,
    "IsActive" BOOLEAN NOT NULL DEFAULT true,
    "CreatedBy" VARCHAR(20) NOT NULL DEFAULT 'system',
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "LastUpdatedBy" VARCHAR(20),
    "UpdateDate" TIMESTAMP
);

-- 6. Identities (Centralized Unpadded Prefix & Sequence Management)
CREATE TABLE IF NOT EXISTS "Identities" (
    "Entry" SERIAL PRIMARY KEY,
    "Domain" VARCHAR(50) NOT NULL UNIQUE,
    "Prefix1" VARCHAR(3) NOT NULL,
    "Prefix2" VARCHAR(3),
    "Prefix3" VARCHAR(3),
    "CurrentSequence" INT NOT NULL DEFAULT 0,
    "IsActive" BOOLEAN NOT NULL DEFAULT true,
    "CreatedBy" VARCHAR(20) NOT NULL DEFAULT 'system',
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "LastUpdatedBy" VARCHAR(20),
    "UpdateDate" TIMESTAMP,
    CONSTRAINT "CHK_Identities_IsActive" CHECK ("IsActive" IN (true, false)),
    CONSTRAINT "CHK_Identities_CurrentSequence" CHECK ("CurrentSequence" >= 0),
    CONSTRAINT "CHK_Identities_Prefix1_Len" CHECK (LENGTH(TRIM("Prefix1")) = 3)
);

CREATE UNIQUE INDEX IF NOT EXISTS "UQ_Identities_Domain_Active" ON "Identities" ("Domain") WHERE "IsActive" = true;

INSERT INTO "Identities" ("Domain", "Prefix1", "CurrentSequence") VALUES
('Venue', 'VNU', 0),
('Event', 'EVT', 0),
('DeparturePoint', 'DEP', 0),
('Route', 'RTE', 0),
('RouteStop', 'STP', 0),
('Schedule', 'SCH', 0),
('Ticket', 'TCK', 0),
('NotificationContact', 'NTC', 0),
('NotificationSetting', 'NTS', 0),
('Reservation', 'RSV', 0),
('PaymentMethod', 'PMT', 4),
('WhatsAppSettings', 'WAS', 0),
('WhatsAppConversation', 'WAC', 0),
('WhatsAppMessage', 'WAM', 0),
('CustomMessage', 'MSG', 0),
('CustomButton', 'BTN', 0),
('CustomAttachment', 'ATT', 0),
('PaymentTransaction', 'TXN', 0),
('StripeSession', 'STS', 0),
('StripeTransaction', 'STX', 0),
('User', 'USR', 0)
ON CONFLICT ("Domain") DO NOTHING;

-- 7. Venues
CREATE TABLE IF NOT EXISTS "Venues" (
    "Entry" SERIAL PRIMARY KEY,
    "Code" VARCHAR(50) NOT NULL UNIQUE,
    "Name" VARCHAR(255) NOT NULL,
    "City" VARCHAR(3) NOT NULL,
    "Address" TEXT NOT NULL,
    "IsActive" BOOLEAN NOT NULL DEFAULT true,
    "CreatedBy" VARCHAR(20) NOT NULL DEFAULT 'system',
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "LastUpdatedBy" VARCHAR(20),
    "UpdateDate" TIMESTAMP,
    CONSTRAINT "CHK_Venues_IsActive" CHECK ("IsActive" IN (true, false))
);

CREATE UNIQUE INDEX IF NOT EXISTS "UQ_Venues_Name_Active" ON "Venues" (LOWER(TRIM("Name"))) WHERE "IsActive" = true;
CREATE INDEX IF NOT EXISTS "IDX_Venues_Code_Active" ON "Venues" ("Code") WHERE "IsActive" = true;

-- 8. Events
CREATE TABLE IF NOT EXISTS "Events" (
    "Entry" SERIAL PRIMARY KEY,
    "Code" VARCHAR(50) NOT NULL UNIQUE,
    "Name" VARCHAR(255) NOT NULL,
    "Date" TIMESTAMP NOT NULL,
    "VenueCode" VARCHAR(50) NOT NULL,
    "Image" TEXT,
    "Category" "EventCategory",
    "IsPromoted" BOOLEAN NOT NULL DEFAULT false,
    "IsUpcoming" BOOLEAN NOT NULL DEFAULT true,
    "CommingSoon" BOOLEAN NOT NULL DEFAULT false,
    "IsActive" BOOLEAN NOT NULL DEFAULT true,
    "CreatedBy" VARCHAR(20) NOT NULL DEFAULT 'system',
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "LastUpdatedBy" VARCHAR(20),
    "UpdateDate" TIMESTAMP,
    CONSTRAINT "CHK_Events_IsPromoted" CHECK ("IsPromoted" IN (true, false)),
    CONSTRAINT "CHK_Events_IsUpcoming" CHECK ("IsUpcoming" IN (true, false)),
    CONSTRAINT "CHK_Events_CommingSoon" CHECK ("CommingSoon" IN (true, false)),
    CONSTRAINT "CHK_Events_IsActive" CHECK ("IsActive" IN (true, false))
);

CREATE UNIQUE INDEX IF NOT EXISTS "UQ_Events_Name_Active" ON "Events" (LOWER(TRIM("Name"))) WHERE "IsActive" = true;
CREATE INDEX IF NOT EXISTS "IDX_Events_VenueCode_Active" ON "Events" ("VenueCode") WHERE "IsActive" = true;
CREATE INDEX IF NOT EXISTS "IDX_Events_Date_Active" ON "Events" ("Date") WHERE "IsActive" = true;
CREATE INDEX IF NOT EXISTS "IDX_Events_Category_Active" ON "Events" ("Category") WHERE "IsActive" = true;

-- 9. DeparturePoints
CREATE TABLE IF NOT EXISTS "DeparturePoints" (
    "Entry" SERIAL PRIMARY KEY,
    "Code" VARCHAR(50) NOT NULL UNIQUE,
    "Name" VARCHAR(255) NOT NULL,
    "City" VARCHAR(3) NOT NULL,
    "Address" TEXT NOT NULL,
    "PointType" "DeparturePointType" NOT NULL,
    "IsActive" BOOLEAN NOT NULL DEFAULT true,
    "CreatedBy" VARCHAR(20) NOT NULL DEFAULT 'system',
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "LastUpdatedBy" VARCHAR(20),
    "UpdateDate" TIMESTAMP,
    CONSTRAINT "CHK_DeparturePoints_IsActive" CHECK ("IsActive" IN (true, false))
);

CREATE UNIQUE INDEX IF NOT EXISTS "UQ_DeparturePoints_Name_Active" ON "DeparturePoints" (LOWER(TRIM("Name"))) WHERE "IsActive" = true;
CREATE INDEX IF NOT EXISTS "IDX_DeparturePoints_City_PointType_Active" ON "DeparturePoints" ("City", "PointType") WHERE "IsActive" = true;

-- 10. Routes
CREATE TABLE IF NOT EXISTS "Routes" (
    "Entry" SERIAL PRIMARY KEY,
    "Code" VARCHAR(50) NOT NULL UNIQUE,
    "Name" VARCHAR(255) NOT NULL,
    "OriginPointCode" VARCHAR(50) NOT NULL,
    "DestinationVenueCode" VARCHAR(50) NOT NULL,
    "BasePrice" NUMERIC(10, 2) NOT NULL,
    "IsActive" BOOLEAN NOT NULL DEFAULT true,
    "CreatedBy" VARCHAR(20) NOT NULL DEFAULT 'system',
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "LastUpdatedBy" VARCHAR(20),
    "UpdateDate" TIMESTAMP,
    CONSTRAINT "CHK_Routes_BasePrice" CHECK ("BasePrice" >= 0),
    CONSTRAINT "CHK_Routes_IsActive" CHECK ("IsActive" IN (true, false))
);

CREATE UNIQUE INDEX IF NOT EXISTS "UQ_Routes_Name_Active" ON "Routes" (LOWER(TRIM("Name"))) WHERE "IsActive" = true;
CREATE INDEX IF NOT EXISTS "IDX_Routes_Origin_Dest_Active" ON "Routes" ("OriginPointCode", "DestinationVenueCode") WHERE "IsActive" = true;

-- 11. RouteStops
CREATE TABLE IF NOT EXISTS "RouteStops" (
    "Entry" SERIAL PRIMARY KEY,
    "Code" VARCHAR(50) NOT NULL,
    "Name" VARCHAR(255) NOT NULL,
    "RouteCode" VARCHAR(50) NOT NULL,
    "Type" "DeparturePointType" NOT NULL,
    "BasePrice" NUMERIC(10, 2) NOT NULL,
    "ArrivalTime" VARCHAR(50),
    "IsActive" BOOLEAN NOT NULL DEFAULT true,
    "CreatedBy" VARCHAR(20) NOT NULL DEFAULT 'system',
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "LastUpdatedBy" VARCHAR(20),
    "UpdateDate" TIMESTAMP,
    CONSTRAINT "CHK_RouteStops_BasePrice" CHECK ("BasePrice" >= 0),
    CONSTRAINT "CHK_RouteStops_IsActive" CHECK ("IsActive" IN (true, false))
);

CREATE INDEX IF NOT EXISTS "IDX_RouteStops_RouteCode_Active" ON "RouteStops" ("RouteCode") WHERE "IsActive" = true;
CREATE UNIQUE INDEX IF NOT EXISTS "UQ_RouteStops_RouteCode_Pickup_Active" ON "RouteStops" ("RouteCode") WHERE "Type" = 'PICKUP' AND "IsActive" = true;
CREATE UNIQUE INDEX IF NOT EXISTS "UQ_RouteStops_RouteCode_Dropoff_Active" ON "RouteStops" ("RouteCode") WHERE "Type" = 'DROPOFF' AND "IsActive" = true;

-- 12. Schedules
CREATE TABLE IF NOT EXISTS "Schedules" (
    "Entry" SERIAL PRIMARY KEY,
    "Code" VARCHAR(50) NOT NULL UNIQUE,
    "Name" VARCHAR(255) NOT NULL,
    "EventCode" VARCHAR(50) NOT NULL,
    "RouteCode" VARCHAR(50),
    "Type" "ScheduleType" NOT NULL,
    "DepartureTime" TIMESTAMP NOT NULL,
    "DepartureLabel" VARCHAR(100) NOT NULL,
    "ArrivalTime" TIMESTAMP,
    "ReturnDepartureTime" TIMESTAMP,
    "ReturnArrivalTime" TIMESTAMP,
    "Duration" INT,
    "Capacity" INT NOT NULL,
    "AvailableSeats" INT NOT NULL,
    "Price" NUMERIC(10, 2) NOT NULL,
    "IsActive" BOOLEAN NOT NULL DEFAULT true,
    "CreatedBy" VARCHAR(20) NOT NULL DEFAULT 'system',
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "LastUpdatedBy" VARCHAR(20),
    "UpdateDate" TIMESTAMP,
    CONSTRAINT "CHK_Schedules_Capacity" CHECK ("Capacity" >= 0),
    CONSTRAINT "CHK_Schedules_AvailableSeats" CHECK ("AvailableSeats" >= 0),
    CONSTRAINT "CHK_Schedules_Price" CHECK ("Price" >= 0),
    CONSTRAINT "CHK_Schedules_AvailableSeats_Capacity" CHECK ("AvailableSeats" <= "Capacity"),
    CONSTRAINT "CHK_Schedules_IsActive" CHECK ("IsActive" IN (true, false))
);

CREATE INDEX IF NOT EXISTS "IDX_Schedules_EventCode_Type_Active" ON "Schedules" ("EventCode", "Type") WHERE "IsActive" = true;
CREATE INDEX IF NOT EXISTS "IDX_Schedules_RouteCode_Active" ON "Schedules" ("RouteCode") WHERE "IsActive" = true;

-- 13. Tickets
CREATE TABLE IF NOT EXISTS "Tickets" (
    "Entry" SERIAL PRIMARY KEY,
    "Code" VARCHAR(50) NOT NULL UNIQUE,
    "Name" VARCHAR(255) NOT NULL,
    "ScheduleCode" VARCHAR(50) NOT NULL,
    "Phone" VARCHAR(50) NOT NULL,
    "Quantity" INT NOT NULL DEFAULT 1,
    "IsActive" BOOLEAN NOT NULL DEFAULT true,
    "CreatedBy" VARCHAR(20) NOT NULL DEFAULT 'system',
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "LastUpdatedBy" VARCHAR(20),
    "UpdateDate" TIMESTAMP,
    CONSTRAINT "CHK_Tickets_Phone" CHECK (LENGTH(TRIM("Phone")) >= 7),
    CONSTRAINT "CHK_Tickets_Quantity" CHECK ("Quantity" > 0),
    CONSTRAINT "CHK_Tickets_IsActive" CHECK ("IsActive" IN (true, false))
);

CREATE INDEX IF NOT EXISTS "IDX_Tickets_ScheduleCode_Active" ON "Tickets" ("ScheduleCode") WHERE "IsActive" = true;

-- 14. NotificationContacts
CREATE TABLE IF NOT EXISTS "NotificationContacts" (
    "Entry" SERIAL PRIMARY KEY,
    "Code" VARCHAR(50) NOT NULL UNIQUE,
    "Name" VARCHAR(255) NOT NULL,
    "Phone" VARCHAR(50) NOT NULL,
    "Role" VARCHAR(100) NOT NULL,
    "IsActive" BOOLEAN NOT NULL DEFAULT true,
    "CreatedBy" VARCHAR(20) NOT NULL DEFAULT 'system',
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "LastUpdatedBy" VARCHAR(20),
    "UpdateDate" TIMESTAMP,
    CONSTRAINT "CHK_NotificationContacts_Phone" CHECK (LENGTH(TRIM("Phone")) >= 7),
    CONSTRAINT "CHK_NotificationContacts_IsActive" CHECK ("IsActive" IN (true, false))
);

CREATE UNIQUE INDEX IF NOT EXISTS "UQ_NotificationContacts_Phone_Active" ON "NotificationContacts" ("Phone") WHERE "IsActive" = true;

-- 15. NotificationSettings
CREATE TABLE IF NOT EXISTS "NotificationSettings" (
    "Entry" SERIAL PRIMARY KEY,
    "Code" VARCHAR(50) NOT NULL DEFAULT 'DEFAULT' UNIQUE,
    "Name" VARCHAR(255) NOT NULL DEFAULT 'Notification Settings',
    "OwnerWhatsapp" VARCHAR(50) NOT NULL,
    "CreatedBy" VARCHAR(20) NOT NULL DEFAULT 'system',
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "LastUpdatedBy" VARCHAR(20),
    "UpdateDate" TIMESTAMP,
    CONSTRAINT "CHK_NotificationSettings_Code" CHECK (LENGTH(TRIM("Code")) > 0),
    CONSTRAINT "CHK_NotificationSettings_OwnerWhatsapp" CHECK (LENGTH(TRIM("OwnerWhatsapp")) >= 7)
);

-- 16. Reservations
CREATE TABLE IF NOT EXISTS "Reservations" (
    "Entry" SERIAL PRIMARY KEY,
    "Code" VARCHAR(50) NOT NULL UNIQUE,
    "FirstName1" VARCHAR(100) NOT NULL,
    "FirstName2" VARCHAR(100),
    "LastName1" VARCHAR(100) NOT NULL,
    "LastName2" VARCHAR(100),
    "Email" VARCHAR(100),
    "Phone" VARCHAR(50) NOT NULL,
    "Seats" INT NOT NULL DEFAULT 1,
    "EventCode" VARCHAR(50) NOT NULL,
    "RouteCode" VARCHAR(50) NOT NULL,
    "PickupPointCode" VARCHAR(50) NOT NULL,
    "DropoffPointCode" VARCHAR(50) NOT NULL,
    "ScheduleCode" VARCHAR(50),
    "TripType" "ScheduleType" NOT NULL DEFAULT 'ROUND_TRIP',
    "UnitPrice" NUMERIC(10, 2) NOT NULL DEFAULT 0.00,
    "TotalPrice" NUMERIC(10, 2) NOT NULL DEFAULT 0.00,
    "PaymentMethod" "PaymentMethodType" NOT NULL DEFAULT 'NOT_APPLICABLE',
    "PaymentStatus" "PaymentStatusType" NOT NULL DEFAULT 'UNPAID',
    "PaymentReference" VARCHAR(100),
    "PaymentDate" TIMESTAMP,
    "CancellationReason" TEXT,
    "CancelledDate" TIMESTAMP,
    "Status" "ReservationStatusType" NOT NULL DEFAULT 'UNCONFIRMED',
    "ExpiresAt" TIMESTAMPTZ,
    "IsActive" BOOLEAN NOT NULL DEFAULT true,
    "CreatedBy" VARCHAR(20) NOT NULL DEFAULT 'system',
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "LastUpdatedBy" VARCHAR(20),
    "UpdateDate" TIMESTAMP,
    CONSTRAINT "CHK_Reservations_FirstName1" CHECK (LENGTH(TRIM("FirstName1")) > 0),
    CONSTRAINT "CHK_Reservations_LastName1" CHECK (LENGTH(TRIM("LastName1")) > 0),
    CONSTRAINT "CHK_Reservations_Phone" CHECK (LENGTH(TRIM("Phone")) >= 7),
    CONSTRAINT "CHK_Reservations_Seats" CHECK ("Seats" > 0),
    CONSTRAINT "CHK_Reservations_UnitPrice" CHECK ("UnitPrice" >= 0),
    CONSTRAINT "CHK_Reservations_TotalPrice" CHECK ("TotalPrice" >= 0),
    CONSTRAINT "CHK_Reservations_IsActive" CHECK ("IsActive" IN (true, false))
);

ALTER TABLE "Reservations" ADD COLUMN IF NOT EXISTS "ExpiresAt" TIMESTAMPTZ;

CREATE INDEX IF NOT EXISTS "IDX_Reservations_EventCode_Active" ON "Reservations" ("EventCode") WHERE "IsActive" = true;
CREATE INDEX IF NOT EXISTS "IDX_Reservations_RouteCode_Active" ON "Reservations" ("RouteCode") WHERE "IsActive" = true;
CREATE INDEX IF NOT EXISTS "IDX_Reservations_Phone_Active" ON "Reservations" ("Phone") WHERE "IsActive" = true;
CREATE INDEX IF NOT EXISTS "IDX_Reservations_Status_Active" ON "Reservations" ("Status") WHERE "IsActive" = true;
CREATE INDEX IF NOT EXISTS "IDX_Reservations_PaymentStatus_Active" ON "Reservations" ("PaymentStatus") WHERE "IsActive" = true;
CREATE INDEX IF NOT EXISTS "IDX_Reservations_PaymentMethod_Active" ON "Reservations" ("PaymentMethod") WHERE "IsActive" = true;
CREATE INDEX IF NOT EXISTS "IDX_Reservations_ExpiresAt_Pending" ON "Reservations" ("ExpiresAt") WHERE "Status" = 'PENDING' AND "IsActive" = true;

-- 17. PaymentMethods
CREATE TABLE IF NOT EXISTS "PaymentMethods" (
    "Entry" SERIAL PRIMARY KEY,
    "Code" VARCHAR(50) NOT NULL UNIQUE,
    "Name" VARCHAR(255) NOT NULL,
    "Type" "PaymentMethodType" NOT NULL DEFAULT 'NOT_APPLICABLE',
    "UsesCommission" BOOLEAN NOT NULL DEFAULT false,
    "CommissionRate" NUMERIC(5, 2) NOT NULL DEFAULT 0.00,
    "UsesIntegration" BOOLEAN NOT NULL DEFAULT false,
    "IntegrationProvider" VARCHAR(50),
    "IsActive" BOOLEAN NOT NULL DEFAULT true,
    "IsCanceled" BOOLEAN NOT NULL DEFAULT false,
    "CreatedBy" VARCHAR(20) NOT NULL DEFAULT 'system',
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "LastUpdatedBy" VARCHAR(20),
    "UpdateDate" TIMESTAMP,
    CONSTRAINT "CHK_PaymentMethods_IsActive" CHECK ("IsActive" IN (true, false))
);

ALTER TABLE "PaymentMethods" ADD COLUMN IF NOT EXISTS "UsesIntegration" BOOLEAN NOT NULL DEFAULT false;
ALTER TABLE "PaymentMethods" ADD COLUMN IF NOT EXISTS "IntegrationProvider" VARCHAR(50);
ALTER TABLE "PaymentMethods" ADD COLUMN IF NOT EXISTS "IsCanceled" BOOLEAN NOT NULL DEFAULT false;

CREATE UNIQUE INDEX IF NOT EXISTS "UQ_PaymentMethods_Name_Active" ON "PaymentMethods" (LOWER(TRIM("Name"))) WHERE "IsActive" = true AND "IsCanceled" = false;

INSERT INTO "PaymentMethods" ("Code", "Name", "Type", "UsesCommission", "CommissionRate", "CreatedBy") VALUES
('PMT1', 'Efectivo', 'CASH', false, 0.00, 'system'),
('PMT2', 'Transferencia', 'TRANSFER', false, 0.00, 'system'),
('PMT3', 'Tarjeta / Otro', 'CARD', true, 3.50, 'system'),
('PMT4', 'No Aplica / Pend.', 'NOT_APPLICABLE', false, 0.00, 'system')
ON CONFLICT ("Code") DO NOTHING;

-- 17.1 PaymentMethodDetails (Detalles bancarios para transferencias/sin integración)
-- Relación lógica vía "Code" con "PaymentMethods"."Code" (Sin FOREIGN KEY)
CREATE TABLE IF NOT EXISTS "PaymentMethodDetails" (
    "Entry"            SERIAL PRIMARY KEY,
    "Code"             VARCHAR(50) NOT NULL,
    "BankName"         VARCHAR(255) NOT NULL,
    "CLABE"            VARCHAR(255) NOT NULL,
    "AccountHolder"    VARCHAR(255) NOT NULL,
    "PaymentReference" VARCHAR(255),
    "IsActive"         BOOLEAN NOT NULL DEFAULT true,
    "CreatedBy" VARCHAR(20) NOT NULL DEFAULT 'system',
    "CreateDate"       TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "LastUpdatedBy"    VARCHAR(20),
    "UpdateDate"       TIMESTAMP,
    CONSTRAINT "CHK_PaymentMethodDetails_IsActive" CHECK ("IsActive" IN (true, false))
);

CREATE INDEX IF NOT EXISTS "IDX_PMDetails_Code" 
    ON "PaymentMethodDetails" ("Code");
CREATE INDEX IF NOT EXISTS "IDX_PMDetails_Active" 
    ON "PaymentMethodDetails" ("Code", "IsActive") WHERE "IsActive" = true;

-- 18. PaymentGateways (Configuración Dinámica Centralizada Multi-Pasarela)
CREATE TABLE IF NOT EXISTS "PaymentGateways" (
    "Entry" SERIAL PRIMARY KEY,
    "Code" VARCHAR(50) NOT NULL UNIQUE,
    "Name" VARCHAR(255) NOT NULL,
    "Provider" VARCHAR(50) NOT NULL, -- 'STRIPE', 'OPENPAY', 'MERCADOPAGO', 'PAYPAL'
    "MerchantId" VARCHAR(255),
    "PublicKey" TEXT,
    "PrivateKey" TEXT,
    "WebhookSecret" TEXT,
    "ApiBaseUrl" VARCHAR(255),
    "Currency" VARCHAR(10) NOT NULL DEFAULT 'mxn',
    "IsTestMode" BOOLEAN NOT NULL DEFAULT true,
    "IsActive" BOOLEAN NOT NULL DEFAULT true,
    "CreatedBy" VARCHAR(20) NOT NULL DEFAULT 'system',
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "LastUpdatedBy" VARCHAR(20),
    "UpdateDate" TIMESTAMP,
    CONSTRAINT "CHK_PaymentGateways_IsActive" CHECK ("IsActive" IN (true, false))
);

CREATE UNIQUE INDEX IF NOT EXISTS "UQ_PaymentGateways_Provider_Active" ON "PaymentGateways" ("Provider") WHERE "IsActive" = true;

INSERT INTO "PaymentGateways" ("Code", "Name", "Provider", "IsTestMode", "IsActive", "CreatedBy") VALUES
('GW_STRIPE', 'Stripe Gateway', 'STRIPE', true, true, 'system'),
('GW_OPENPAY', 'OpenPay México', 'OPENPAY', true, false, 'system'),
('GW_MERCADOPAGO', 'Mercado Pago Checkout Pro', 'MERCADOPAGO', true, false, 'system')
ON CONFLICT ("Code") DO NOTHING;

-- 19. PaymentTransactions (Auditoría Universal de Transacciones de Cobro)
CREATE TABLE IF NOT EXISTS "PaymentTransactions" (
    "Entry" SERIAL PRIMARY KEY,
    "Code" VARCHAR(50) NOT NULL UNIQUE,
    "EntityType" VARCHAR(50) NOT NULL DEFAULT 'ROUTE_RESERVATION',
    "EntityCode" VARCHAR(50) NOT NULL,
    "Provider" VARCHAR(50) NOT NULL, -- 'STRIPE', 'OPENPAY', 'MERCADOPAGO'
    "PaymentMethod" VARCHAR(50) NOT NULL DEFAULT 'CARD',
    "Amount" NUMERIC(10, 2) NOT NULL,
    "Currency" VARCHAR(10) NOT NULL DEFAULT 'mxn',
    "Status" VARCHAR(50) NOT NULL DEFAULT 'PENDING', -- 'PENDING', 'SUCCEEDED', 'FAILED', 'EXPIRED', 'CANCELLED'
    "PaymentIntentId" VARCHAR(255),
    "SessionId" VARCHAR(255),
    "Clabe" VARCHAR(50),
    "BankName" VARCHAR(100),
    "ReceiptUrl" TEXT,
    "HostedUrl" TEXT,
    "RawPayload" TEXT,
    "ExpiresAt" TIMESTAMPTZ,
    "IsActive" BOOLEAN NOT NULL DEFAULT true,
    "CreatedBy" VARCHAR(20) NOT NULL DEFAULT 'system',
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "LastUpdatedBy" VARCHAR(20),
    "UpdateDate" TIMESTAMP
);

CREATE INDEX IF NOT EXISTS "IDX_PaymentTransactions_Entity" ON "PaymentTransactions" ("EntityType", "EntityCode");
CREATE INDEX IF NOT EXISTS "IDX_PaymentTransactions_Status" ON "PaymentTransactions" ("Status");
CREATE INDEX IF NOT EXISTS "IDX_PaymentTransactions_ExpiresAt_Pending" ON "PaymentTransactions" ("ExpiresAt") WHERE "Status" = 'PENDING' AND "IsActive" = true;
CREATE INDEX IF NOT EXISTS "IDX_PaymentTransactions_PaymentIntentId" ON "PaymentTransactions" ("PaymentIntentId");
CREATE INDEX IF NOT EXISTS "IDX_PaymentTransactions_Clabe" ON "PaymentTransactions" ("Clabe");

-- 20. OpenPaySettings
CREATE TABLE IF NOT EXISTS "OpenPaySettings" (
    "Entry" SERIAL PRIMARY KEY,
    "Code" VARCHAR(50) NOT NULL DEFAULT 'DEFAULT' UNIQUE,
    "Name" VARCHAR(255) NOT NULL DEFAULT 'OpenPay Settings',
    "MerchantId" VARCHAR(255),
    "PublicKey" TEXT,
    "PrivateKey" TEXT,
    "WebhookSecretKey" TEXT,
    "ApiBaseUrl" VARCHAR(255) NOT NULL DEFAULT 'https://sandbox-api.openpay.mx/v1',
    "IsTestMode" BOOLEAN NOT NULL DEFAULT true,
    "IsActive" BOOLEAN NOT NULL DEFAULT true,
    "CreatedBy" VARCHAR(20) NOT NULL DEFAULT 'system',
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "LastUpdatedBy" VARCHAR(20),
    "UpdateDate" TIMESTAMP
);

INSERT INTO "OpenPaySettings" ("Code", "Name", "IsTestMode", "IsActive", "CreatedBy") VALUES
('DEFAULT', 'OpenPay Settings', true, true, 'system')
ON CONFLICT ("Code") DO NOTHING;

-- 21. MercadoPagoSettings
CREATE TABLE IF NOT EXISTS "MercadoPagoSettings" (
    "Entry" SERIAL PRIMARY KEY,
    "Code" VARCHAR(50) NOT NULL DEFAULT 'DEFAULT' UNIQUE,
    "Name" VARCHAR(255) NOT NULL DEFAULT 'Mercado Pago Settings',
    "PublicKey" TEXT,
    "AccessToken" TEXT,
    "WebhookSecretKey" TEXT,
    "ApiBaseUrl" VARCHAR(255) NOT NULL DEFAULT 'https://api.mercadopago.com',
    "IsTestMode" BOOLEAN NOT NULL DEFAULT true,
    "IsActive" BOOLEAN NOT NULL DEFAULT true,
    "CreatedBy" VARCHAR(20) NOT NULL DEFAULT 'system',
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "LastUpdatedBy" VARCHAR(20),
    "UpdateDate" TIMESTAMP
);

INSERT INTO "MercadoPagoSettings" ("Code", "Name", "IsTestMode", "IsActive", "CreatedBy") VALUES
('DEFAULT', 'Mercado Pago Settings', true, true, 'system')
ON CONFLICT ("Code") DO NOTHING;

-- 22. StripeSettings
CREATE TABLE IF NOT EXISTS "StripeSettings" (
    "Entry" SERIAL PRIMARY KEY,
    "Code" VARCHAR(50) NOT NULL DEFAULT 'DEFAULT' UNIQUE,
    "Name" VARCHAR(255) NOT NULL DEFAULT 'Stripe Payment Settings',
    "PublishableKey" TEXT,
    "SecretKey" TEXT,
    "WebhookSecretKey" TEXT,
    "ApiBaseUrl" VARCHAR(255) NOT NULL DEFAULT 'https://api.stripe.com/v1',
    "CheckoutEndpoint" VARCHAR(255) NOT NULL DEFAULT '/checkout/sessions',
    "WebhookPath" VARCHAR(255) NOT NULL DEFAULT '/api/v1/stripe/webhook',
    "Currency" VARCHAR(10) NOT NULL DEFAULT 'mxn',
    "IsTestMode" BOOLEAN NOT NULL DEFAULT true,
    "IsActive" BOOLEAN NOT NULL DEFAULT true,
    "CreatedBy" VARCHAR(20) NOT NULL DEFAULT 'system',
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "LastUpdatedBy" VARCHAR(20),
    "UpdateDate" TIMESTAMP
);

-- 23. StripeSessions (Checkout Session Tracking)
CREATE TABLE IF NOT EXISTS "StripeSessions" (
    "Entry" SERIAL PRIMARY KEY,
    "Code" VARCHAR(50) NOT NULL UNIQUE,
    "ReservationCode" VARCHAR(50) NOT NULL,
    "StripeSessionId" VARCHAR(255) NOT NULL,
    "PaymentIntentId" VARCHAR(255),
    "CheckoutUrl" TEXT NOT NULL,
    "Amount" NUMERIC(10, 2) NOT NULL,
    "Currency" VARCHAR(10) NOT NULL DEFAULT 'mxn',
    "Status" VARCHAR(50) NOT NULL DEFAULT 'open',
    "IsActive" BOOLEAN NOT NULL DEFAULT true,
    "CreatedBy" VARCHAR(20) NOT NULL DEFAULT 'system',
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "LastUpdatedBy" VARCHAR(20),
    "UpdateDate" TIMESTAMP
);

CREATE INDEX IF NOT EXISTS "IDX_StripeSessions_ReservationCode" ON "StripeSessions" ("ReservationCode");
CREATE INDEX IF NOT EXISTS "IDX_StripeSessions_StripeSessionId" ON "StripeSessions" ("StripeSessionId");

-- 24. StripeTransactions (Detailed Stripe Auditing)
CREATE TABLE IF NOT EXISTS "StripeTransactions" (
    "Entry" SERIAL PRIMARY KEY,
    "Code" VARCHAR(50) NOT NULL UNIQUE,
    "ReservationCode" VARCHAR(50) NOT NULL,
    "PaymentIntentId" VARCHAR(255) NOT NULL,
    "ChargeId" VARCHAR(255),
    "Amount" NUMERIC(10, 2) NOT NULL,
    "Currency" VARCHAR(10) NOT NULL DEFAULT 'mxn',
    "Status" VARCHAR(50) NOT NULL DEFAULT 'succeeded',
    "PaymentMethodType" VARCHAR(50) DEFAULT 'card',
    "Clabe" VARCHAR(50),
    "BankName" VARCHAR(100),
    "CardBrand" VARCHAR(50),
    "CardLast4" VARCHAR(10),
    "CardType" VARCHAR(50),
    "AuthorizationCode" VARCHAR(100),
    "CardFingerprint" VARCHAR(255),
    "ReceiptUrl" TEXT,
    "HostedInstructionsUrl" TEXT,
    "ClientIp" VARCHAR(50),
    "IsActive" BOOLEAN NOT NULL DEFAULT true,
    "CreatedBy" VARCHAR(20) NOT NULL DEFAULT 'system',
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "LastUpdatedBy" VARCHAR(20),
    "UpdateDate" TIMESTAMP
);

ALTER TABLE "StripeTransactions" ADD COLUMN IF NOT EXISTS "PaymentMethodType" VARCHAR(50) DEFAULT 'card';
ALTER TABLE "StripeTransactions" ADD COLUMN IF NOT EXISTS "Clabe" VARCHAR(50);
ALTER TABLE "StripeTransactions" ADD COLUMN IF NOT EXISTS "BankName" VARCHAR(100);
ALTER TABLE "StripeTransactions" ADD COLUMN IF NOT EXISTS "HostedInstructionsUrl" TEXT;

CREATE INDEX IF NOT EXISTS "IDX_StripeTransactions_ReservationCode" ON "StripeTransactions" ("ReservationCode");
CREATE INDEX IF NOT EXISTS "IDX_StripeTransactions_PaymentIntentId" ON "StripeTransactions" ("PaymentIntentId");
CREATE INDEX IF NOT EXISTS "IDX_StripeTransactions_Clabe" ON "StripeTransactions" ("Clabe");

-- 25. WhatsAppSettings (Meta Cloud API)
CREATE TABLE IF NOT EXISTS "WhatsAppSettings" (
    "Entry" SERIAL PRIMARY KEY,
    "Code" VARCHAR(50) NOT NULL UNIQUE DEFAULT 'DEFAULT',
    "Name" VARCHAR(255) NOT NULL DEFAULT 'MetaConfig',
    "PhoneId" VARCHAR(255),
    "ApiToken" TEXT,
    "BusinessAccountId" VARCHAR(255),
    "WebhookVerifyToken" TEXT,
    "ApiVersion" VARCHAR(50) NOT NULL DEFAULT 'v24.0',
    "IsActive" BOOLEAN NOT NULL DEFAULT true,
    "CreatedBy" VARCHAR(20) NOT NULL DEFAULT 'system',
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "LastUpdatedBy" VARCHAR(20),
    "UpdateDate" TIMESTAMP
);

INSERT INTO "WhatsAppSettings" ("Code", "Name", "PhoneId", "ApiToken", "BusinessAccountId", "WebhookVerifyToken", "ApiVersion", "IsActive", "CreatedBy") VALUES
('DEFAULT', 'MetaConfig', '', '', '', '', 'v24.0', true, 'system')
ON CONFLICT ("Code") DO NOTHING;

-- 26. WhatsAppConversations
CREATE TABLE IF NOT EXISTS "WhatsAppConversations" (
    "Entry" SERIAL PRIMARY KEY,
    "Code" VARCHAR(50) NOT NULL UNIQUE,
    "CustomerPhone" VARCHAR(50) NOT NULL,
    "CustomerName" VARCHAR(255),
    "LastMessageText" TEXT,
    "LastMessageDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "UnreadCount" INT NOT NULL DEFAULT 0,
    "Status" VARCHAR(50) NOT NULL DEFAULT 'OPEN',
    "IsActive" BOOLEAN NOT NULL DEFAULT true,
    "CreatedBy" VARCHAR(20) NOT NULL DEFAULT 'system',
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "LastUpdatedBy" VARCHAR(20),
    "UpdateDate" TIMESTAMP
);

CREATE INDEX IF NOT EXISTS "IDX_WhatsAppConversations_CustomerPhone" ON "WhatsAppConversations" ("CustomerPhone");

-- 27. WhatsAppMessages
CREATE TABLE IF NOT EXISTS "WhatsAppMessages" (
    "Entry" SERIAL PRIMARY KEY,
    "Code" VARCHAR(50) NOT NULL UNIQUE,
    "WhatsAppId" VARCHAR(255),
    "ConversationEntry" INT NOT NULL,
    "SenderType" VARCHAR(50) NOT NULL DEFAULT 'OUTBOUND',
    "MessageType" VARCHAR(50) NOT NULL DEFAULT 'TEXT',
    "TemplateName" VARCHAR(255),
    "Content" TEXT,
    "MediaUrl" TEXT,
    "Status" VARCHAR(50) NOT NULL DEFAULT 'SENT',
    "ResponsePayload" TEXT,
    "ErrorMessage" TEXT,
    "SentBy" INT NOT NULL DEFAULT 1,
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP
);

ALTER TABLE "WhatsAppMessages" ADD COLUMN IF NOT EXISTS "WhatsAppId" VARCHAR(255);

CREATE INDEX IF NOT EXISTS "IDX_WhatsAppMessages_ConversationEntry" ON "WhatsAppMessages" ("ConversationEntry");
CREATE INDEX IF NOT EXISTS "IDX_WhatsAppMessages_Code" ON "WhatsAppMessages" ("Code");
CREATE INDEX IF NOT EXISTS "IDX_WhatsAppMessages_WhatsAppId" ON "WhatsAppMessages" ("WhatsAppId");

-- 28. WhatsAppTemplates (Plantillas Oficiales Meta Cloud API)
CREATE TABLE IF NOT EXISTS "WhatsAppTemplates" (
    "Entry" SERIAL PRIMARY KEY,
    "Code" VARCHAR(50) NOT NULL UNIQUE,
    "TemplateName" VARCHAR(100) NOT NULL,
    "Language" VARCHAR(10) NOT NULL DEFAULT 'es_MX',
    "Category" VARCHAR(50) NOT NULL DEFAULT 'UTILITY',
    "HeaderType" VARCHAR(20) NOT NULL DEFAULT 'NONE',
    "BodyTemplate" TEXT NOT NULL,
    "FooterText" VARCHAR(255),
    "ButtonsJson" TEXT,
    "Status" VARCHAR(50) NOT NULL DEFAULT 'APPROVED',
    "IsActive" BOOLEAN NOT NULL DEFAULT true,
    "CreatedBy" VARCHAR(20) NOT NULL DEFAULT 'system',
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "LastUpdatedBy" VARCHAR(20),
    "UpdateDate" TIMESTAMP
);

CREATE INDEX IF NOT EXISTS "IDX_WhatsAppTemplates_TemplateName" ON "WhatsAppTemplates" ("TemplateName");

-- 29. CustomMessages (Motor Dinámico de Plantillas de Mensajes)
CREATE TABLE IF NOT EXISTS "CustomMessages" (
    "Entry" SERIAL PRIMARY KEY,
    "Code" VARCHAR(50) NOT NULL UNIQUE,
    "Title" VARCHAR(255) NOT NULL,
    "MessageType" VARCHAR(50) NOT NULL DEFAULT 'TEXT',
    "HeaderType" VARCHAR(20) NOT NULL DEFAULT 'NONE',
    "HeaderContent" TEXT,
    "BodyTemplate" TEXT NOT NULL,
    "FooterText" VARCHAR(255),
    "MetaTemplateId" VARCHAR(100),
    "MetaStatus" VARCHAR(50) DEFAULT 'NONE',
    "MetaCategory" VARCHAR(50) DEFAULT 'UTILITY',
    "MetaRejectReason" TEXT,
    "IsActive" BOOLEAN NOT NULL DEFAULT true,
    "CreatedBy" VARCHAR(20) NOT NULL DEFAULT 'system',
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "LastUpdatedBy" VARCHAR(20),
    "UpdateDate" TIMESTAMP
);

ALTER TABLE "CustomMessages" ADD COLUMN IF NOT EXISTS "MetaTemplateId" VARCHAR(100);
ALTER TABLE "CustomMessages" ADD COLUMN IF NOT EXISTS "MetaStatus" VARCHAR(50) DEFAULT 'NONE';
ALTER TABLE "CustomMessages" ADD COLUMN IF NOT EXISTS "MetaCategory" VARCHAR(50) DEFAULT 'UTILITY';
ALTER TABLE "CustomMessages" ADD COLUMN IF NOT EXISTS "MetaRejectReason" TEXT;

CREATE INDEX IF NOT EXISTS "IDX_CustomMessages_Code" ON "CustomMessages" ("Code");

-- 30. CustomMessageParameters
CREATE TABLE IF NOT EXISTS "CustomMessageParameters" (
    "Entry" SERIAL PRIMARY KEY,
    "MessageCode" VARCHAR(50) NOT NULL,
    "ParamKey" VARCHAR(50) NOT NULL,
    "ParamName" VARCHAR(100) NOT NULL,
    "DataType" VARCHAR(30) NOT NULL DEFAULT 'STRING',
    "DefaultValue" VARCHAR(255),
    "IsRequired" BOOLEAN NOT NULL DEFAULT true,
    "Description" TEXT,
    "SortOrder" INT NOT NULL DEFAULT 1,
    "CreatedBy" VARCHAR(20) NOT NULL DEFAULT 'system',
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    CONSTRAINT "FK_MsgParams_MessageCode" FOREIGN KEY ("MessageCode") REFERENCES "CustomMessages"("Code") ON DELETE CASCADE,
    CONSTRAINT "UQ_MsgParams_Key" UNIQUE ("MessageCode", "ParamKey")
);

CREATE INDEX IF NOT EXISTS "IDX_CustomMessageParameters_MessageCode" ON "CustomMessageParameters" ("MessageCode");

-- 31. CustomButtons & CustomAttachments
CREATE TABLE IF NOT EXISTS "CustomButtons" (
    "Entry" SERIAL PRIMARY KEY,
    "MessageEntry" INT NOT NULL REFERENCES "CustomMessages"("Entry") ON DELETE CASCADE,
    "ButtonId" VARCHAR(50) NOT NULL,
    "Title" VARCHAR(50) NOT NULL,
    "ActionType" VARCHAR(50) NOT NULL DEFAULT 'TRIGGER_MESSAGE',
    "ActionPayload" TEXT,
    "SortOrder" INT NOT NULL DEFAULT 1,
    "CreatedBy" VARCHAR(20) NOT NULL DEFAULT 'system',
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP
);

CREATE INDEX IF NOT EXISTS "IDX_CustomButtons_MessageEntry" ON "CustomButtons" ("MessageEntry");

CREATE TABLE IF NOT EXISTS "CustomAttachments" (
    "Entry" SERIAL PRIMARY KEY,
    "MessageEntry" INT NOT NULL REFERENCES "CustomMessages"("Entry") ON DELETE CASCADE,
    "MediaType" VARCHAR(50) NOT NULL,
    "MediaUrl" TEXT NOT NULL,
    "CreatedBy" VARCHAR(20) NOT NULL DEFAULT 'system',
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP
);

CREATE INDEX IF NOT EXISTS "IDX_CustomAttachments_MessageEntry" ON "CustomAttachments" ("MessageEntry");

-- Seed Default Internal Custom Templates
DELETE FROM "CustomButtons" WHERE "MessageEntry" IN (SELECT "Entry" FROM "CustomMessages" WHERE "Code" IN ('TPL_WELCOME_WITH_RESERVATION', 'TPL_WELCOME_PROMPT', 'TPL_RESERVATION_DETAILS', 'TPL_NOT_FOUND_ERROR', 'TPL_CARD_PAYMENT_SUCCESS', 'TPL_CARD_PAYMENT_FAILED', 'TPL_TRANSFER_INSTRUCTIONS', 'TPL_TRANSFER_APPROVED', 'TPL_TRANSFER_REJECTED'));
DELETE FROM "CustomMessageParameters" WHERE "MessageCode" IN ('TPL_WELCOME_WITH_RESERVATION', 'TPL_WELCOME_PROMPT', 'TPL_RESERVATION_DETAILS', 'TPL_NOT_FOUND_ERROR', 'TPL_CARD_PAYMENT_SUCCESS', 'TPL_CARD_PAYMENT_FAILED', 'TPL_TRANSFER_INSTRUCTIONS', 'TPL_TRANSFER_APPROVED', 'TPL_TRANSFER_REJECTED');
DELETE FROM "CustomMessages" WHERE "Code" IN ('TPL_WELCOME_WITH_RESERVATION', 'TPL_WELCOME_PROMPT', 'TPL_RESERVATION_DETAILS', 'TPL_NOT_FOUND_ERROR', 'TPL_CARD_PAYMENT_SUCCESS', 'TPL_CARD_PAYMENT_FAILED', 'TPL_TRANSFER_INSTRUCTIONS', 'TPL_TRANSFER_APPROVED', 'TPL_TRANSFER_REJECTED');

INSERT INTO "CustomMessages" ("Code", "Title", "MessageType", "BodyTemplate", "CreatedBy") VALUES
('TPL_WELCOME_WITH_RESERVATION', 'Bienvenida a Cliente Reconocido', 'INTERACTIVE_BUTTON', 
'¡Hola {nombre_registrado}! 👋 Qué gusto saludarte.

Vemos que tienes una reservación activa para:
🎪 *{evento}*
🎟️ Folio: *{folio}*
📍 Origen: *{parada_inicial}*
⏰ Salida: *{hora_salida}*
💳 Estatus: *{estatus}*

¿Deseas consultar los detalles completos de tu viaje o necesitas ayuda adicional?', 'system'),

('TPL_WELCOME_PROMPT', 'Bienvenida a Cliente No Registrado', 'TEXT',
'¡Hola {nombre_cliente}! 👋 Bienvenido al asistente de {empresa} 🚌✨

No encontramos reservaciones activas vinculadas a este número de WhatsApp.

Si compraste con otro número o deseas consultar tu boleto:
🎟️ Escribe el folio de tu boleto (ejemplo: *{ejemplo_folio}*), o
📱 Los *10 dígitos* del número celular con el que te registraste.', 'system'),

('TPL_RESERVATION_DETAILS', 'Detalle Completo de Reservación', 'TEXT',
'📋 *DETALLES DE TU RESERVACIÓN* 🚌✨

🎟️ *Folio:* {folio}
📱 *Teléfono:* {numero_registrado}
👤 *Pasajero:* {nombre_registrado}
💺 *Asientos:* {numero_asientos}
🎪 *Evento:* {evento}
📍 *Origen:* {parada_inicial}
🏁 *Destino:* {destino}
⏰ *Hora de Salida:* {hora_salida}
🏁 *Hora Estimada de Llegada:* {hora_llegada}
💰 *Importe Pagado:* {importe_pagado}
💳 *Forma de Pago:* {detalle_pago}

✅ *Estatus:* {estatus}

¡Te esperamos puntualmente en tu punto de abordaje! 🎒', 'system'),

('TPL_NOT_FOUND_ERROR', 'Reservación No Encontrada', 'TEXT',
'Lo sentimos {nombre_cliente}, no pudimos encontrar ninguna reservación activa con los datos ingresados: "{dato_ingresado}". 🔍

Por favor verifica que:
• El folio inicie con *RSV* (ejemplo: *{ejemplo_folio}*), o
• Hayas escrito los *10 dígitos* del número celular registrado.

Si necesitas ayuda personalizada, nuestro equipo con gusto te atenderá.', 'system'),

('TPL_CARD_PAYMENT_SUCCESS', 'Pago con Tarjeta Exitoso', 'TEXT',
'¡Tu pago con tarjeta ha sido exitoso! 🎉💳

Hola {nombre_registrado}, confirmamos la acreditación de tu pago:
🎟️ *Folio:* {folio}
🎪 *Evento:* {evento}
💺 *Asientos:* {numero_asientos}
💰 *Monto Pagado:* {monto_pagado}
💳 *Tarjeta:* {marca_tarjeta} terminación •••• {ultimos_4_digitos}
📍 *Salida:* {parada_inicial} ({hora_salida})

✅ *Estatus:* CONFIRMADO Y PAGADO

¡Tu lugar está 100% asegurado! Nos vemos en el evento. 🚌✨', 'system'),

('TPL_CARD_PAYMENT_FAILED', 'Pago con Tarjeta Declinado', 'TEXT',
'Aviso sobre el pago de tu reservación ⚠️💳

Hola {nombre_registrado}, no pudimos procesar el cobro con tu tarjeta para el folio *{folio}*:
💰 Monto: *{monto_pagado}*
Motivo: {motivo_fallo}

Para no perder tus lugares, por favor intenta con otra tarjeta o cambia tu forma de pago a Transferencia bancaria.', 'system'),

('TPL_TRANSFER_INSTRUCTIONS', 'Instrucciones de Transferencia Bancaria', 'TEXT',
'Instrucciones para Pago por Transferencia 🏦📋

Hola {nombre_registrado}, para completar tu reservación *{folio}*, realiza tu transferencia con los siguientes datos:

🏦 *Banco:* {banco}
👤 *Beneficiario:* {beneficiario}
🔢 *CLABE:* `{clabe}`
💰 *Monto Exacto:* *{monto_a_pagar}*
📝 *Concepto / Referencia:* `{folio}`

📸 *Importante:* Una vez realizada, envía la captura de tu comprobante por este chat para validar y asegurar tus lugares.', 'system'),

('TPL_TRANSFER_APPROVED', 'Transferencia Acreditada', 'TEXT',
'¡Tu transferencia ha sido verificada con éxito! 🎉✅

Hola {nombre_registrado}, validamos tu comprobante de pago:
🎟️ *Folio:* {folio}
🎪 *Evento:* {evento}
💺 *Asientos:* {numero_asientos}
💰 *Monto Acreditado:* {monto_pagado}
📍 *Salida:* {parada_inicial} ({hora_salida})

✅ *Estatus:* CONFIRMADO Y PAGADO

¡Tu viaje está confirmado! Te esperamos en tu punto de abordaje. 🚌✨', 'system'),

('TPL_TRANSFER_REJECTED', 'Comprobante de Transferencia Rechazado', 'TEXT',
'Aviso sobre tu comprobante de transferencia ⚠️📄

Hola {nombre_registrado}, tuvimos un inconveniente al validar tu comprobante para el folio *{folio}*:
Motivo: {motivo_rechazo}

Por favor envía un nuevo comprobante legible o comunícate por este chat para asistirte.', 'system')
ON CONFLICT ("Code") DO NOTHING;

-- Seed Buttons for TPL_WELCOME_WITH_RESERVATION
INSERT INTO "CustomButtons" ("MessageEntry", "ButtonId", "Title", "ActionType", "SortOrder", "CreatedBy")
SELECT m."Entry", 'BTN_DETAILS_{folio}', 'Ver Detalles', 'TRIGGER_MESSAGE', 1, 'system'
FROM "CustomMessages" m WHERE m."Code" = 'TPL_WELCOME_WITH_RESERVATION'
ON CONFLICT DO NOTHING;

-- Seed Parameters for TPL_WELCOME_WITH_RESERVATION
INSERT INTO "CustomMessageParameters" ("MessageCode", "ParamKey", "ParamName", "DataType", "DefaultValue", "IsRequired", "SortOrder") VALUES
('TPL_WELCOME_WITH_RESERVATION', 'nombre_registrado', 'Nombre del Pasajero', 'STRING', 'estimado(a) cliente', true, 1),
('TPL_WELCOME_WITH_RESERVATION', 'evento', 'Nombre del Evento', 'STRING', 'Evento General', true, 2),
('TPL_WELCOME_WITH_RESERVATION', 'folio', 'Folio de Reservación', 'STRING', 'RSV1', true, 3),
('TPL_WELCOME_WITH_RESERVATION', 'parada_inicial', 'Parada de Salida', 'STRING', 'Punto de Abordaje', true, 4),
('TPL_WELCOME_WITH_RESERVATION', 'hora_salida', 'Hora de Salida', 'TIME', 'Por confirmar', true, 5),
('TPL_WELCOME_WITH_RESERVATION', 'estatus', 'Estatus de la Reserva', 'STRING', 'CONFIRMADO', true, 6)
ON CONFLICT ("MessageCode", "ParamKey") DO NOTHING;

-- Seed Parameters for TPL_WELCOME_PROMPT
INSERT INTO "CustomMessageParameters" ("MessageCode", "ParamKey", "ParamName", "DataType", "DefaultValue", "IsRequired", "SortOrder") VALUES
('TPL_WELCOME_PROMPT', 'nombre_cliente', 'Nombre del Perfil', 'STRING', 'estimado(a) cliente', false, 1),
('TPL_WELCOME_PROMPT', 'empresa', 'Nombre de Empresa', 'STRING', 'OmniRoute', true, 2),
('TPL_WELCOME_PROMPT', 'ejemplo_folio', 'Ejemplo de Folio', 'STRING', 'RSV1', true, 3)
ON CONFLICT ("MessageCode", "ParamKey") DO NOTHING;

-- Seed Parameters for TPL_RESERVATION_DETAILS
INSERT INTO "CustomMessageParameters" ("MessageCode", "ParamKey", "ParamName", "DataType", "DefaultValue", "IsRequired", "SortOrder") VALUES
('TPL_RESERVATION_DETAILS', 'folio', 'Folio de Reserva', 'STRING', 'RSV1', true, 1),
('TPL_RESERVATION_DETAILS', 'numero_registrado', 'Teléfono Registrado', 'STRING', '', true, 2),
('TPL_RESERVATION_DETAILS', 'nombre_registrado', 'Nombre del Pasajero', 'STRING', 'Pasajero', true, 3),
('TPL_RESERVATION_DETAILS', 'numero_asientos', 'Número de Asientos', 'INTEGER', '1', true, 4),
('TPL_RESERVATION_DETAILS', 'evento', 'Nombre del Evento', 'STRING', 'Evento', true, 5),
('TPL_RESERVATION_DETAILS', 'parada_inicial', 'Parada de Salida', 'STRING', 'Origen', true, 6),
('TPL_RESERVATION_DETAILS', 'destino', 'Destino Final', 'STRING', 'Destino', true, 7),
('TPL_RESERVATION_DETAILS', 'hora_salida', 'Hora de Salida', 'TIME', '00:00', true, 8),
('TPL_RESERVATION_DETAILS', 'hora_llegada', 'Hora Estimada de Llegada', 'TIME', '00:00', false, 9),
('TPL_RESERVATION_DETAILS', 'importe_pagado', 'Monto Pagado', 'CURRENCY', '$0.00 MXN', true, 10),
('TPL_RESERVATION_DETAILS', 'detalle_pago', 'Detalle de Pago', 'STRING', 'Pago General', true, 11),
('TPL_RESERVATION_DETAILS', 'estatus', 'Estatus de la Reserva', 'STRING', 'CONFIRMADO', true, 12)
ON CONFLICT ("MessageCode", "ParamKey") DO NOTHING;

-- Seed Parameters for TPL_NOT_FOUND_ERROR
INSERT INTO "CustomMessageParameters" ("MessageCode", "ParamKey", "ParamName", "DataType", "DefaultValue", "IsRequired", "SortOrder") VALUES
('TPL_NOT_FOUND_ERROR', 'nombre_cliente', 'Nombre del Perfil', 'STRING', 'estimado(a) cliente', false, 1),
('TPL_NOT_FOUND_ERROR', 'dato_ingresado', 'Dato Ingresado', 'STRING', '', true, 2),
('TPL_NOT_FOUND_ERROR', 'ejemplo_folio', 'Ejemplo de Folio', 'STRING', 'RSV1', true, 3)
ON CONFLICT ("MessageCode", "ParamKey") DO NOTHING;

-- Seed Parameters for TPL_CARD_PAYMENT_SUCCESS
INSERT INTO "CustomMessageParameters" ("MessageCode", "ParamKey", "ParamName", "DataType", "DefaultValue", "IsRequired", "SortOrder") VALUES
('TPL_CARD_PAYMENT_SUCCESS', 'nombre_registrado', 'Nombre del Pasajero', 'STRING', 'Pasajero', true, 1),
('TPL_CARD_PAYMENT_SUCCESS', 'folio', 'Folio de Reserva', 'STRING', '', true, 2),
('TPL_CARD_PAYMENT_SUCCESS', 'evento', 'Nombre del Evento', 'STRING', '', true, 3),
('TPL_CARD_PAYMENT_SUCCESS', 'numero_asientos', 'Número de Asientos', 'INTEGER', '1', true, 4),
('TPL_CARD_PAYMENT_SUCCESS', 'monto_pagado', 'Monto Pagado', 'CURRENCY', '$0.00 MXN', true, 5),
('TPL_CARD_PAYMENT_SUCCESS', 'marca_tarjeta', 'Marca de Tarjeta', 'STRING', 'Tarjeta', true, 6),
('TPL_CARD_PAYMENT_SUCCESS', 'ultimos_4_digitos', 'Últimos 4 Dígitos', 'MASKED_CARD', '••••', true, 7),
('TPL_CARD_PAYMENT_SUCCESS', 'parada_inicial', 'Parada de Salida', 'STRING', '', true, 8),
('TPL_CARD_PAYMENT_SUCCESS', 'hora_salida', 'Hora de Salida', 'TIME', '', true, 9)
ON CONFLICT ("MessageCode", "ParamKey") DO NOTHING;

-- Seed Parameters for TPL_CARD_PAYMENT_FAILED
INSERT INTO "CustomMessageParameters" ("MessageCode", "ParamKey", "ParamName", "DataType", "DefaultValue", "IsRequired", "SortOrder") VALUES
('TPL_CARD_PAYMENT_FAILED', 'nombre_registrado', 'Nombre del Pasajero', 'STRING', 'Pasajero', true, 1),
('TPL_CARD_PAYMENT_FAILED', 'folio', 'Folio de Reserva', 'STRING', '', true, 2),
('TPL_CARD_PAYMENT_FAILED', 'monto_pagado', 'Monto Intentado', 'CURRENCY', '$0.00 MXN', true, 3),
('TPL_CARD_PAYMENT_FAILED', 'motivo_fallo', 'Motivo del Fallo', 'STRING', 'Fondos insuficientes o declinada', true, 4)
ON CONFLICT ("MessageCode", "ParamKey") DO NOTHING;

-- Seed Parameters for TPL_TRANSFER_INSTRUCTIONS
INSERT INTO "CustomMessageParameters" ("MessageCode", "ParamKey", "ParamName", "DataType", "DefaultValue", "IsRequired", "SortOrder") VALUES
('TPL_TRANSFER_INSTRUCTIONS', 'nombre_registrado', 'Nombre del Pasajero', 'STRING', 'Pasajero', true, 1),
('TPL_TRANSFER_INSTRUCTIONS', 'folio', 'Folio de Reserva', 'STRING', '', true, 2),
('TPL_TRANSFER_INSTRUCTIONS', 'banco', 'Banco Destino', 'STRING', 'BBVA', true, 3),
('TPL_TRANSFER_INSTRUCTIONS', 'beneficiario', 'Nombre del Beneficiario', 'STRING', 'OmniRoute Transportes S.A. de C.V.', true, 4),
('TPL_TRANSFER_INSTRUCTIONS', 'clabe', 'CLABE Interbancaria', 'STRING', '012180001234567890', true, 5),
('TPL_TRANSFER_INSTRUCTIONS', 'monto_a_pagar', 'Monto a Pagar', 'CURRENCY', '$0.00 MXN', true, 6)
ON CONFLICT ("MessageCode", "ParamKey") DO NOTHING;

-- Seed Parameters for TPL_TRANSFER_APPROVED
INSERT INTO "CustomMessageParameters" ("MessageCode", "ParamKey", "ParamName", "DataType", "DefaultValue", "IsRequired", "SortOrder") VALUES
('TPL_TRANSFER_APPROVED', 'nombre_registrado', 'Nombre del Pasajero', 'STRING', 'Pasajero', true, 1),
('TPL_TRANSFER_APPROVED', 'folio', 'Folio de Reserva', 'STRING', '', true, 2),
('TPL_TRANSFER_APPROVED', 'evento', 'Nombre del Evento', 'STRING', '', true, 3),
('TPL_TRANSFER_APPROVED', 'numero_asientos', 'Número de Asientos', 'INTEGER', '1', true, 4),
('TPL_TRANSFER_APPROVED', 'monto_pagado', 'Monto Acreditado', 'CURRENCY', '$0.00 MXN', true, 5),
('TPL_TRANSFER_APPROVED', 'parada_inicial', 'Parada de Salida', 'STRING', '', true, 6),
('TPL_TRANSFER_APPROVED', 'hora_salida', 'Hora de Salida', 'TIME', '', true, 7)
ON CONFLICT ("MessageCode", "ParamKey") DO NOTHING;

-- Seed Parameters for TPL_TRANSFER_REJECTED
INSERT INTO "CustomMessageParameters" ("MessageCode", "ParamKey", "ParamName", "DataType", "DefaultValue", "IsRequired", "SortOrder") VALUES
('TPL_TRANSFER_REJECTED', 'nombre_registrado', 'Nombre del Pasajero', 'STRING', 'Pasajero', true, 1),
('TPL_TRANSFER_REJECTED', 'folio', 'Folio de Reserva', 'STRING', '', true, 2),
('TPL_TRANSFER_REJECTED', 'motivo_rechazo', 'Motivo del Rechazo', 'STRING', 'Comprobante ilegible o monto incompleto', true, 3)
ON CONFLICT ("MessageCode", "ParamKey") DO NOTHING;

-- =============================================================================
-- 32. DATA MIGRATION & CODE NORMALIZATION SCRIPT
-- Strips leading zeroes from code formats (e.g., EVT001 -> EVT1, VNU002 -> VNU2)
-- across all tables and foreign key relationships.
-- =============================================================================
DO $$
BEGIN
    -- A. Update Foreign Key references first to avoid constraints mismatch

    -- Events -> Venues
    IF EXISTS (SELECT 1 FROM information_schema.tables WHERE table_name = 'Events') THEN
        UPDATE "Events" 
        SET "VenueCode" = REGEXP_REPLACE("VenueCode", '^([A-Za-z]+)0+([1-9][0-9]*)$', '\1\2')
        WHERE "VenueCode" ~ '^([A-Za-z]+)0+([1-9][0-9]*)$';
    END IF;

    -- Routes -> DeparturePoints & Venues
    IF EXISTS (SELECT 1 FROM information_schema.tables WHERE table_name = 'Routes') THEN
        UPDATE "Routes" 
        SET "OriginPointCode" = REGEXP_REPLACE("OriginPointCode", '^([A-Za-z]+)0+([1-9][0-9]*)$', '\1\2')
        WHERE "OriginPointCode" ~ '^([A-Za-z]+)0+([1-9][0-9]*)$';

        UPDATE "Routes" 
        SET "DestinationVenueCode" = REGEXP_REPLACE("DestinationVenueCode", '^([A-Za-z]+)0+([1-9][0-9]*)$', '\1\2')
        WHERE "DestinationVenueCode" ~ '^([A-Za-z]+)0+([1-9][0-9]*)$';
    END IF;

    -- RouteStops -> Routes
    IF EXISTS (SELECT 1 FROM information_schema.tables WHERE table_name = 'RouteStops') THEN
        UPDATE "RouteStops" 
        SET "RouteCode" = REGEXP_REPLACE("RouteCode", '^([A-Za-z]+)0+([1-9][0-9]*)$', '\1\2')
        WHERE "RouteCode" ~ '^([A-Za-z]+)0+([1-9][0-9]*)$';
    END IF;

    -- Schedules -> Events & Routes
    IF EXISTS (SELECT 1 FROM information_schema.tables WHERE table_name = 'Schedules') THEN
        UPDATE "Schedules" 
        SET "EventCode" = REGEXP_REPLACE("EventCode", '^([A-Za-z]+)0+([1-9][0-9]*)$', '\1\2')
        WHERE "EventCode" ~ '^([A-Za-z]+)0+([1-9][0-9]*)$';

        UPDATE "Schedules" 
        SET "RouteCode" = REGEXP_REPLACE("RouteCode", '^([A-Za-z]+)0+([1-9][0-9]*)$', '\1\2')
        WHERE "RouteCode" ~ '^([A-Za-z]+)0+([1-9][0-9]*)$';
    END IF;

    -- Tickets -> Schedules
    IF EXISTS (SELECT 1 FROM information_schema.tables WHERE table_name = 'Tickets') THEN
        UPDATE "Tickets" 
        SET "ScheduleCode" = REGEXP_REPLACE("ScheduleCode", '^([A-Za-z]+)0+([1-9][0-9]*)$', '\1\2')
        WHERE "ScheduleCode" ~ '^([A-Za-z]+)0+([1-9][0-9]*)$';
    END IF;

    -- Reservations -> Events, Routes, DeparturePoints, Schedules
    IF EXISTS (SELECT 1 FROM information_schema.tables WHERE table_name = 'Reservations') THEN
        UPDATE "Reservations" 
        SET "EventCode" = REGEXP_REPLACE("EventCode", '^([A-Za-z]+)0+([1-9][0-9]*)$', '\1\2')
        WHERE "EventCode" ~ '^([A-Za-z]+)0+([1-9][0-9]*)$';

        UPDATE "Reservations" 
        SET "RouteCode" = REGEXP_REPLACE("RouteCode", '^([A-Za-z]+)0+([1-9][0-9]*)$', '\1\2')
        WHERE "RouteCode" ~ '^([A-Za-z]+)0+([1-9][0-9]*)$';

        UPDATE "Reservations" 
        SET "PickupPointCode" = REGEXP_REPLACE("PickupPointCode", '^([A-Za-z]+)0+([1-9][0-9]*)$', '\1\2')
        WHERE "PickupPointCode" ~ '^([A-Za-z]+)0+([1-9][0-9]*)$';

        UPDATE "Reservations" 
        SET "DropoffPointCode" = REGEXP_REPLACE("DropoffPointCode", '^([A-Za-z]+)0+([1-9][0-9]*)$', '\1\2')
        WHERE "DropoffPointCode" ~ '^([A-Za-z]+)0+([1-9][0-9]*)$';

        UPDATE "Reservations" 
        SET "ScheduleCode" = REGEXP_REPLACE("ScheduleCode", '^([A-Za-z]+)0+([1-9][0-9]*)$', '\1\2')
        WHERE "ScheduleCode" ~ '^([A-Za-z]+)0+([1-9][0-9]*)$';
    END IF;

    -- StripeSessions -> Reservations
    IF EXISTS (SELECT 1 FROM information_schema.tables WHERE table_name = 'StripeSessions') THEN
        UPDATE "StripeSessions"
        SET "ReservationCode" = REGEXP_REPLACE("ReservationCode", '^([A-Za-z]+)0+([1-9][0-9]*)$', '\1\2')
        WHERE "ReservationCode" ~ '^([A-Za-z]+)0+([1-9][0-9]*)$';
    END IF;

    -- StripeTransactions -> Reservations
    IF EXISTS (SELECT 1 FROM information_schema.tables WHERE table_name = 'StripeTransactions') THEN
        UPDATE "StripeTransactions"
        SET "ReservationCode" = REGEXP_REPLACE("ReservationCode", '^([A-Za-z]+)0+([1-9][0-9]*)$', '\1\2')
        WHERE "ReservationCode" ~ '^([A-Za-z]+)0+([1-9][0-9]*)$';
    END IF;

    -- PaymentTransactions -> EntityCode
    IF EXISTS (SELECT 1 FROM information_schema.tables WHERE table_name = 'PaymentTransactions') THEN
        UPDATE "PaymentTransactions"
        SET "EntityCode" = REGEXP_REPLACE("EntityCode", '^([A-Za-z]+)0+([1-9][0-9]*)$', '\1\2')
        WHERE "EntityCode" ~ '^([A-Za-z]+)0+([1-9][0-9]*)$';
    END IF;

    -- Sessions -> Users
    IF EXISTS (SELECT 1 FROM information_schema.tables WHERE table_name = 'Sessions') THEN
        UPDATE "Sessions"
        SET "UserCode" = REGEXP_REPLACE("UserCode", '^([A-Za-z]+)0+([1-9][0-9]*)$', '\1\2')
        WHERE "UserCode" ~ '^([A-Za-z]+)0+([1-9][0-9]*)$';
    END IF;

    -- B. Update Primary Key Codes across all entities
    IF EXISTS (SELECT 1 FROM information_schema.tables WHERE table_name = 'Venues') THEN
        UPDATE "Venues" SET "Code" = REGEXP_REPLACE("Code", '^([A-Za-z]+)0+([1-9][0-9]*)$', '\1\2') WHERE "Code" ~ '^([A-Za-z]+)0+([1-9][0-9]*)$';
    END IF;

    IF EXISTS (SELECT 1 FROM information_schema.tables WHERE table_name = 'Events') THEN
        UPDATE "Events" SET "Code" = REGEXP_REPLACE("Code", '^([A-Za-z]+)0+([1-9][0-9]*)$', '\1\2') WHERE "Code" ~ '^([A-Za-z]+)0+([1-9][0-9]*)$';
    END IF;

    IF EXISTS (SELECT 1 FROM information_schema.tables WHERE table_name = 'DeparturePoints') THEN
        UPDATE "DeparturePoints" SET "Code" = REGEXP_REPLACE("Code", '^([A-Za-z]+)0+([1-9][0-9]*)$', '\1\2') WHERE "Code" ~ '^([A-Za-z]+)0+([1-9][0-9]*)$';
    END IF;

    IF EXISTS (SELECT 1 FROM information_schema.tables WHERE table_name = 'Routes') THEN
        UPDATE "Routes" SET "Code" = REGEXP_REPLACE("Code", '^([A-Za-z]+)0+([1-9][0-9]*)$', '\1\2') WHERE "Code" ~ '^([A-Za-z]+)0+([1-9][0-9]*)$';
    END IF;

    IF EXISTS (SELECT 1 FROM information_schema.tables WHERE table_name = 'RouteStops') THEN
        UPDATE "RouteStops" SET "Code" = REGEXP_REPLACE("Code", '^([A-Za-z]+)0+([1-9][0-9]*)$', '\1\2') WHERE "Code" ~ '^([A-Za-z]+)0+([1-9][0-9]*)$';
    END IF;

    IF EXISTS (SELECT 1 FROM information_schema.tables WHERE table_name = 'Schedules') THEN
        UPDATE "Schedules" SET "Code" = REGEXP_REPLACE("Code", '^([A-Za-z]+)0+([1-9][0-9]*)$', '\1\2') WHERE "Code" ~ '^([A-Za-z]+)0+([1-9][0-9]*)$';
    END IF;

    IF EXISTS (SELECT 1 FROM information_schema.tables WHERE table_name = 'Tickets') THEN
        UPDATE "Tickets" SET "Code" = REGEXP_REPLACE("Code", '^([A-Za-z]+)0+([1-9][0-9]*)$', '\1\2') WHERE "Code" ~ '^([A-Za-z]+)0+([1-9][0-9]*)$';
    END IF;

    IF EXISTS (SELECT 1 FROM information_schema.tables WHERE table_name = 'NotificationContacts') THEN
        UPDATE "NotificationContacts" SET "Code" = REGEXP_REPLACE("Code", '^([A-Za-z]+)0+([1-9][0-9]*)$', '\1\2') WHERE "Code" ~ '^([A-Za-z]+)0+([1-9][0-9]*)$';
    END IF;

    IF EXISTS (SELECT 1 FROM information_schema.tables WHERE table_name = 'Reservations') THEN
        UPDATE "Reservations" SET "Code" = REGEXP_REPLACE("Code", '^([A-Za-z]+)0+([1-9][0-9]*)$', '\1\2') WHERE "Code" ~ '^([A-Za-z]+)0+([1-9][0-9]*)$';
    END IF;

    IF EXISTS (SELECT 1 FROM information_schema.tables WHERE table_name = 'PaymentMethods') THEN
        UPDATE "PaymentMethods" SET "Code" = REGEXP_REPLACE("Code", '^([A-Za-z]+)0+([1-9][0-9]*)$', '\1\2') WHERE "Code" ~ '^([A-Za-z]+)0+([1-9][0-9]*)$';
    END IF;

    IF EXISTS (SELECT 1 FROM information_schema.tables WHERE table_name = 'Users') THEN
        UPDATE "Users" SET "Code" = REGEXP_REPLACE("Code", '^([A-Za-z]+)0+([1-9][0-9]*)$', '\1\2') WHERE "Code" ~ '^([A-Za-z]+)0+([1-9][0-9]*)$';
    END IF;

    IF EXISTS (SELECT 1 FROM information_schema.tables WHERE table_name = 'StripeSessions') THEN
        UPDATE "StripeSessions" SET "Code" = REGEXP_REPLACE("Code", '^([A-Za-z]+)0+([1-9][0-9]*)$', '\1\2') WHERE "Code" ~ '^([A-Za-z]+)0+([1-9][0-9]*)$';
    END IF;

    IF EXISTS (SELECT 1 FROM information_schema.tables WHERE table_name = 'StripeTransactions') THEN
        UPDATE "StripeTransactions" SET "Code" = REGEXP_REPLACE("Code", '^([A-Za-z]+)0+([1-9][0-9]*)$', '\1\2') WHERE "Code" ~ '^([A-Za-z]+)0+([1-9][0-9]*)$';
    END IF;

    IF EXISTS (SELECT 1 FROM information_schema.tables WHERE table_name = 'PaymentTransactions') THEN
        UPDATE "PaymentTransactions" SET "Code" = REGEXP_REPLACE("Code", '^([A-Za-z]+)0+([1-9][0-9]*)$', '\1\2') WHERE "Code" ~ '^([A-Za-z]+)0+([1-9][0-9]*)$';
    END IF;

    IF EXISTS (SELECT 1 FROM information_schema.tables WHERE table_name = 'WhatsAppConversations') THEN
        UPDATE "WhatsAppConversations" SET "Code" = 'WAC' || "Entry" WHERE "Code" LIKE 'CONV-%' OR "Code" ~ '^([A-Za-z]+)0+([1-9][0-9]*)$';
    END IF;

    IF EXISTS (SELECT 1 FROM information_schema.tables WHERE table_name = 'WhatsAppMessages') THEN
        UPDATE "WhatsAppMessages" SET "WhatsAppId" = "Code" WHERE ("WhatsAppId" IS NULL OR "WhatsAppId" = '') AND "Code" LIKE 'wamid.%';
        UPDATE "WhatsAppMessages" SET "Code" = 'WAM' || "Entry" WHERE "Code" LIKE 'wamid.%' OR "Code" LIKE 'ERR-%' OR "Code" ~ '^([A-Za-z]+)0+([1-9][0-9]*)$';
    END IF;
END $$;

-- =============================================================================
-- 34. SystemLicenses
--     Almacena la OmniLicense API Key activa para persistencia entre reinicios.
--     La VALIDACIÓN criptográfica se realiza en RAM (LicenseService / C++).
--     Solo una licencia puede estar activa (IsActive = true) a la vez.
-- =============================================================================
CREATE TABLE IF NOT EXISTS "SystemLicenses" (
    "Entry"       SERIAL PRIMARY KEY,
    "Code"        VARCHAR(50) NOT NULL UNIQUE,
    "ApiKey"      TEXT NOT NULL,
    "ClientName"  VARCHAR(255) NOT NULL DEFAULT 'OmniSphere Licensee',
    "Issuer"      VARCHAR(255) NOT NULL DEFAULT 'OmniSphere Authority',
    "IssuedAt"    DATE,
    "ExpiresAt"   DATE NOT NULL,
    "Modules"     TEXT NOT NULL DEFAULT '[]',
    "IsActive"    BOOLEAN NOT NULL DEFAULT true,
    "CreatedBy" VARCHAR(20) NOT NULL DEFAULT 'system',
    "CreateDate"  TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "UpdateDate"  TIMESTAMP
);

CREATE UNIQUE INDEX IF NOT EXISTS "UQ_SystemLicenses_Active"
    ON "SystemLicenses" ("IsActive") WHERE "IsActive" = true;

-- Semilla de identidad para la generación de códigos LIC1, LIC2...
INSERT INTO "Identities" ("Domain", "Prefix1", "CurrentSequence")
VALUES ('SystemLicense', 'LIC', 0)
ON CONFLICT ("Domain") DO NOTHING;

-- =============================================================================
-- 35. Access Control & Security (Modules, Permissions, Roles, RolePermissions, UserPermissions, AuthorizationAuditLog)
-- =============================================================================

ALTER TABLE "Users" ADD COLUMN IF NOT EXISTS "RoleCode" VARCHAR(50);
ALTER TABLE "Users" ADD COLUMN IF NOT EXISTS "IsCanceled" BOOLEAN NOT NULL DEFAULT false;

-- Identities seed for Roles, Permissions, Modules
INSERT INTO "Identities" ("Domain", "Prefix1", "CurrentSequence") VALUES
('Role', 'ROL', 0),
('Permission', 'PRM', 0),
('Module', 'MOD', 0)
ON CONFLICT ("Domain") DO NOTHING;

-- A. Modules
CREATE TABLE IF NOT EXISTS "Modules" (
    "Entry" SERIAL PRIMARY KEY,
    "Code" VARCHAR(50) NOT NULL UNIQUE,
    "Name" VARCHAR(100) NOT NULL UNIQUE,
    "Description" TEXT,
    "Icon" VARCHAR(50) NOT NULL DEFAULT 'folder',
    "SortOrder" INT NOT NULL DEFAULT 1,
    "IsActive" BOOLEAN NOT NULL DEFAULT true,
    "CreatedBy" VARCHAR(20) NOT NULL DEFAULT 'system',
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "LastUpdatedBy" VARCHAR(20),
    "UpdateDate" TIMESTAMP
);

-- B. Permissions
CREATE TABLE IF NOT EXISTS "Permissions" (
    "Entry" SERIAL PRIMARY KEY,
    "Code" VARCHAR(50) NOT NULL UNIQUE,
    "Name" VARCHAR(150) NOT NULL,
    "Description" TEXT,
    "ModuleCode" VARCHAR(50) NOT NULL,
    "IsActive" BOOLEAN NOT NULL DEFAULT true,
    "CreatedBy" VARCHAR(20) NOT NULL DEFAULT 'system',
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "LastUpdatedBy" VARCHAR(20),
    "UpdateDate" TIMESTAMP
);
CREATE INDEX IF NOT EXISTS "IDX_Permissions_ModuleCode" ON "Permissions" ("ModuleCode");

-- C. Roles
CREATE TABLE IF NOT EXISTS "Roles" (
    "Entry" SERIAL PRIMARY KEY,
    "Code" VARCHAR(50) NOT NULL UNIQUE,
    "Name" VARCHAR(100) NOT NULL UNIQUE,
    "Description" TEXT,
    "IsActive" BOOLEAN NOT NULL DEFAULT true,
    "IsCanceled" BOOLEAN NOT NULL DEFAULT false,
    "CreatedBy" VARCHAR(20) NOT NULL DEFAULT 'system',
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "LastUpdatedBy" VARCHAR(20),
    "UpdateDate" TIMESTAMP
);

-- D. RolePermissions
CREATE TABLE IF NOT EXISTS "RolePermissions" (
    "Entry" SERIAL PRIMARY KEY,
    "RoleCode" VARCHAR(50) NOT NULL,
    "PermissionCode" VARCHAR(50) NOT NULL,
    "ModuleCode" VARCHAR(50) NOT NULL,
    "IsAllowed" BOOLEAN NOT NULL DEFAULT true,
    "IsActive" BOOLEAN NOT NULL DEFAULT true,
    "CreatedBy" VARCHAR(20) NOT NULL DEFAULT 'system',
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "LastUpdatedBy" VARCHAR(20),
    "UpdateDate" TIMESTAMP,
    CONSTRAINT "UQ_RolePermissions" UNIQUE ("RoleCode", "PermissionCode")
);
CREATE INDEX IF NOT EXISTS "IDX_RolePermissions_RoleCode" ON "RolePermissions" ("RoleCode");

-- E. UserPermissions
CREATE TABLE IF NOT EXISTS "UserPermissions" (
    "Entry" SERIAL PRIMARY KEY,
    "UserCode" VARCHAR(50) NOT NULL,
    "PermissionCode" VARCHAR(50) NOT NULL,
    "ModuleCode" VARCHAR(50) NOT NULL,
    "IsAllowed" BOOLEAN NOT NULL DEFAULT true,
    "GrantedByCode" VARCHAR(50),
    "IsActive" BOOLEAN NOT NULL DEFAULT true,
    "CreatedBy" VARCHAR(20) NOT NULL DEFAULT 'system',
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "LastUpdatedBy" VARCHAR(20),
    "UpdateDate" TIMESTAMP,
    CONSTRAINT "UQ_UserPermissions" UNIQUE ("UserCode", "PermissionCode")
);
CREATE INDEX IF NOT EXISTS "IDX_UserPermissions_UserCode" ON "UserPermissions" ("UserCode");

-- F. AuthorizationAuditLog
CREATE TABLE IF NOT EXISTS "AuthorizationAuditLog" (
    "Entry" SERIAL PRIMARY KEY,
    "UserCode" VARCHAR(50),
    "GrantedByCode" VARCHAR(50),
    "Module" VARCHAR(50),
    "Permission" VARCHAR(100),
    "ResourceCode" VARCHAR(50),
    "Status" BOOLEAN NOT NULL,
    "Reason" TEXT,
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP
);
CREATE INDEX IF NOT EXISTS "IDX_AuthAudit_UserCode" ON "AuthorizationAuditLog" ("UserCode");
CREATE INDEX IF NOT EXISTS "IDX_AuthAudit_CreateDate" ON "AuthorizationAuditLog" ("CreateDate");

-- Seed Modules
INSERT INTO "Modules" ("Code", "Name", "Description", "Icon", "SortOrder") VALUES
('MOD_OVERVIEW', 'Panel Analítico', 'Métricas y estadísticas globales', 'dashboard', 1),
('MOD_RESERVATIONS', 'Reservaciones', 'Control de boletos y pasajeros', 'confirmation_number', 2),
('MOD_SCHEDULES', 'Horarios y Salidas', 'Programación de corridas y viajes', 'schedule', 3),
('MOD_EVENTS', 'Eventos', 'Conciertos, festivales y eventos', 'event', 4),
('MOD_VENUES', 'Destinos', 'Recintos, estadios y destinos finales', 'flag', 5),
('MOD_POINTS', 'Orígenes y Paradas', 'Puntos de abordaje y paradas intermedias', 'place', 6),
('MOD_ROUTES', 'Rutas de Viaje', 'Trazado de rutas y tarifas base', 'alt_route', 7),
('MOD_PAYMENTS', 'Formas de Pago', 'Métodos de pago y cuentas bancarias', 'payments', 8),
('MOD_INTEGRATIONS', 'Integraciones', 'WhatsApp Cloud API y Stripe', 'hub', 9),
('MOD_SETTINGS', 'Configuración General', 'Parámetros del sistema y licencias', 'settings', 10),
('MOD_USERS', 'Personal y Accesos', 'Gestión de colaboradores, roles y permisos', 'badge', 11)
ON CONFLICT ("Code") DO NOTHING;

-- Seed Permissions in Spanish
INSERT INTO "Permissions" ("Code", "Name", "Description", "ModuleCode") VALUES
-- 01. Panel Analítico
('MODULE_OVERVIEW_ACCESS', 'Acceso a Estadísticas', 'Permite visualizar el panel de control y métricas globales', 'MOD_OVERVIEW'),
('ROUTE_OVERVIEW_EXPORT', 'Exportar Informes', 'Permite descargar reportes ejecutivos en formato Excel/PDF', 'MOD_OVERVIEW'),

-- 02. Reservaciones
('MODULE_RESERVATIONS_ACCESS', 'Acceso a Reservaciones', 'Permite ingresar a la lista de reservaciones y boletos', 'MOD_RESERVATIONS'),
('ROUTE_RESERVATION_CREATE', 'Registrar Reservación', 'Permite crear nuevas reservaciones manuales en taquilla', 'MOD_RESERVATIONS'),
('ROUTE_RESERVATION_UPDATE', 'Modificar Reservación', 'Permite cambiar datos de pasajeros, asientos o paradas', 'MOD_RESERVATIONS'),
('ROUTE_RESERVATION_STATUS', 'Actualizar Estatus', 'Permite cambiar el estado (Confirmar, Completar, etc.)', 'MOD_RESERVATIONS'),
('ROUTE_RESERVATION_PAY', 'Procesar Cobros', 'Permite registrar pagos en efectivo, tarjeta o validar transferencias', 'MOD_RESERVATIONS'),
('ROUTE_RESERVATION_CANCEL', 'Cancelar Reservación', 'Permite anular una reservación liberando los asientos ocupados', 'MOD_RESERVATIONS'),
('ROUTE_RESERVATION_DELETE', 'Eliminar Registro', 'Permite dar de baja lógica la reservación del sistema', 'MOD_RESERVATIONS'),

-- 03. Horarios y Salidas
('MODULE_SCHEDULES_ACCESS', 'Acceso a Horarios', 'Permite visualizar los horarios y salidas programadas', 'MOD_SCHEDULES'),
('ROUTE_SCHEDULE_CREATE', 'Programar Salida', 'Permite dar de alta una nueva corrida o salida programada', 'MOD_SCHEDULES'),
('ROUTE_SCHEDULE_UPDATE', 'Editar Salida', 'Permite modificar horas de salida, capacidad y precios', 'MOD_SCHEDULES'),
('ROUTE_SCHEDULE_DELETE', 'Cancelar/Eliminar Salida', 'Permite anular o eliminar corridas programadas', 'MOD_SCHEDULES'),

-- 04. Eventos
('MODULE_EVENTS_ACCESS', 'Acceso a Eventos', 'Permite consultar el catálogo de conciertos y eventos', 'MOD_EVENTS'),
('ROUTE_EVENT_CREATE', 'Crear Evento', 'Permite registrar nuevos eventos con fecha y recinto', 'MOD_EVENTS'),
('ROUTE_EVENT_UPDATE', 'Editar Evento', 'Permite modificar la información, imagen o categoría del evento', 'MOD_EVENTS'),
('ROUTE_EVENT_DELETE', 'Eliminar Evento', 'Permite dar de baja un evento del sistema', 'MOD_EVENTS'),

-- 05. Destinos (Venues)
('MODULE_VENUES_ACCESS', 'Acceso a Destinos', 'Permite consultar recintos, estadios y destinos', 'MOD_VENUES'),
('ROUTE_VENUE_CREATE', 'Registrar Destino', 'Permite dar de alta nuevos destinos y ubicaciones', 'MOD_VENUES'),
('ROUTE_VENUE_UPDATE', 'Modificar Destino', 'Permite editar dirección, ciudad y nombre del destino', 'MOD_VENUES'),
('ROUTE_VENUE_DELETE', 'Eliminar Destino', 'Permite eliminar destinos registrados', 'MOD_VENUES'),

-- 06. Orígenes y Paradas
('MODULE_POINTS_ACCESS', 'Acceso a Paradas', 'Permite consultar orígenes, puntos de abordaje y paradas', 'MOD_POINTS'),
('ROUTE_POINT_CREATE', 'Crear Punto de Abordaje', 'Permite dar de alta nuevas paradas y terminales', 'MOD_POINTS'),
('ROUTE_POINT_UPDATE', 'Editar Parada', 'Permite modificar dirección y tipo de punto de abordaje', 'MOD_POINTS'),
('ROUTE_POINT_DELETE', 'Eliminar Parada', 'Permite retirar paradas del catálogo', 'MOD_POINTS'),

-- 07. Rutas de Viaje
('MODULE_ROUTES_ACCESS', 'Acceso a Rutas', 'Permite ver las rutas trazadas entre origen y destino', 'MOD_ROUTES'),
('ROUTE_ROUTE_CREATE', 'Trazar Nueva Ruta', 'Permite vincular origen y destino con precio base', 'MOD_ROUTES'),
('ROUTE_ROUTE_UPDATE', 'Modificar Ruta', 'Permite cambiar itinerarios, paradas intermedias y tarifas', 'MOD_ROUTES'),
('ROUTE_ROUTE_DELETE', 'Eliminar Ruta', 'Permite dar de baja rutas de viaje', 'MOD_ROUTES'),

-- 08. Formas de Pago
('MODULE_PAYMENTS_ACCESS', 'Acceso a Métodos de Pago', 'Permite ver las opciones y cuentas bancarias configuradas', 'MOD_PAYMENTS'),
('PAYMENTS_METHOD_CREATE', 'Crear Método de Pago', 'Permite habilitar nuevas formas de pago en el checkout', 'MOD_PAYMENTS'),
('PAYMENTS_METHOD_UPDATE', 'Editar Configuración de Pago', 'Permite modificar comisiones y pasarelas vinculadas', 'MOD_PAYMENTS'),
('PAYMENTS_BANK_MANAGE', 'Administrar Cuentas Bancarias', 'Permite editar CLABEs, bancos y beneficiarios de transferencia', 'MOD_PAYMENTS'),
('PAYMENTS_METHOD_DELETE', 'Eliminar Método de Pago', 'Permite desactivar o eliminar opciones de pago', 'MOD_PAYMENTS'),

-- 09. Integraciones
('MODULE_INTEGRATIONS_ACCESS', 'Acceso a Integraciones', 'Permite ver el estado de WhatsApp y pasarelas externas', 'MOD_INTEGRATIONS'),
('INTEGRATION_STRIPE_MANAGE', 'Configurar Stripe', 'Permite editar claves de API y webhooks de Stripe', 'MOD_INTEGRATIONS'),
('INTEGRATION_WHATSAPP_MANAGE', 'Configurar WhatsApp API', 'Permite vincular tokens de Meta Cloud y plantillas', 'MOD_INTEGRATIONS'),
('INTEGRATION_TEST', 'Pruebas de Diagnóstico', 'Permite enviar mensajes y cobros de prueba', 'MOD_INTEGRATIONS'),
('NOTIF_STAFF_MANAGE', 'Notificaciones a Personal', 'Permite configurar alertas de salida a choferes y staff', 'MOD_INTEGRATIONS'),

-- 10. Configuración
('MODULE_SETTINGS_ACCESS', 'Acceso a Configuración', 'Permite visualizar parámetros globales y sistema', 'MOD_SETTINGS'),
('ROUTE_CONFIG_UPDATE', 'Modificar Parámetros', 'Permite cambiar datos de la empresa, impuestos y divisas', 'MOD_SETTINGS'),

-- 11. Personal y Roles
('MODULE_USERS_ACCESS', 'Acceso a Personal', 'Permite acceder a la administración de usuarios y accesos', 'MOD_USERS'),
('CORE_USER_CREATE', 'Registrar Empleado/Usuario', 'Permite dar de alta nuevos colaboradores y cuentas de acceso', 'MOD_USERS'),
('CORE_USER_UPDATE', 'Modificar Usuario', 'Permite editar datos personales, teléfonos y estatus', 'MOD_USERS'),
('CORE_USER_DELETE', 'Eliminar Usuario', 'Permite dar de baja un usuario del sistema', 'MOD_USERS'),
('CORE_ROLE_MANAGE', 'Administrar Roles', 'Permite crear o ajustar perfiles de seguridad predefinidos', 'MOD_USERS'),
('CORE_PERM_MANAGE', 'Modificar Permisos', 'Permite otorgar o revocar permisos específicos a usuarios', 'MOD_USERS')
ON CONFLICT ("Code") DO NOTHING;

-- Migration: Ensure all CreatedBy and LastUpdatedBy columns are VARCHAR(20) on existing databases
DO $$
DECLARE
    tbl text;
    tables text[] := ARRAY[
        'Users', 'Employees', 'Identities', 'Roles', 'Permissions', 'RolePermissions', 'UserPermissions',
        'Departments', 'AuthorizationTemplates', 'AuthorizationStages', 'AuthorizationRules', 
        'AuthorizationRuleUsers', 'AuthorizationRequests', 'AuthorizationDecisions',
        'PaymentMethods', 'PaymentMethodDetails', 'PaymentGateways', 'PaymentTransactions',
        'StripeSettings', 'OpenPaySettings', 'MercadoPagoSettings', 'EmailSettings', 'SystemConfigs',
        'WhatsAppSettings', 'TwilioSettings', 'CustomMessages', 'CustomButtons',
        'Branches', 'TaxRates', 'UnitOfMeasures', 'Currencies', 'ExchangeRates', 'Series'
    ];
BEGIN
    FOREACH tbl IN ARRAY tables
    LOOP
        IF EXISTS (SELECT 1 FROM information_schema.tables WHERE table_schema = 'public' AND table_name = tbl) THEN
            IF EXISTS (SELECT 1 FROM information_schema.columns WHERE table_schema = 'public' AND table_name = tbl AND column_name = 'CreatedBy' AND data_type != 'character varying') THEN
                BEGIN
                    EXECUTE format('ALTER TABLE %I ALTER COLUMN "CreatedBy" DROP DEFAULT', tbl);
                EXCEPTION WHEN OTHERS THEN NULL;
                END;
                EXECUTE format('ALTER TABLE %I ALTER COLUMN "CreatedBy" TYPE VARCHAR(20) USING CASE WHEN "CreatedBy"::TEXT IN (''0'', ''1'', '''', ''SYSTEM'') THEN ''system'' WHEN "CreatedBy" IS NULL THEN ''system'' ELSE LOWER("CreatedBy"::TEXT) END', tbl);
                EXECUTE format('ALTER TABLE %I ALTER COLUMN "CreatedBy" SET DEFAULT ''system''', tbl);
                EXECUTE format('ALTER TABLE %I ALTER COLUMN "CreatedBy" SET NOT NULL', tbl);
            END IF;

            IF EXISTS (SELECT 1 FROM information_schema.columns WHERE table_schema = 'public' AND table_name = tbl AND column_name = 'LastUpdatedBy' AND data_type != 'character varying') THEN
                BEGIN
                    EXECUTE format('ALTER TABLE %I ALTER COLUMN "LastUpdatedBy" DROP DEFAULT', tbl);
                EXCEPTION WHEN OTHERS THEN NULL;
                END;
                EXECUTE format('ALTER TABLE %I ALTER COLUMN "LastUpdatedBy" TYPE VARCHAR(20) USING CASE WHEN "LastUpdatedBy" IS NULL OR "LastUpdatedBy"::TEXT IN (''0'', '''') THEN NULL WHEN "LastUpdatedBy"::TEXT IN (''1'', ''SYSTEM'') THEN ''system'' ELSE LOWER("LastUpdatedBy"::TEXT) END', tbl);
            END IF;

            BEGIN
                EXECUTE format('UPDATE %I SET "CreatedBy" = ''system'' WHERE "CreatedBy" = ''SYSTEM''', tbl);
                EXECUTE format('UPDATE %I SET "LastUpdatedBy" = ''system'' WHERE "LastUpdatedBy" = ''SYSTEM''', tbl);
            EXCEPTION WHEN OTHERS THEN NULL;
            END;
        END IF;
    END LOOP;
END $$;

