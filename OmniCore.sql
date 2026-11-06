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
    "EmployeeCode" VARCHAR(50),
    "Department" INT,
    "PermissionMode" "PermissionMode" NOT NULL DEFAULT 'P',
    "RoleEntry" INT,
    "RoleCode" VARCHAR(50),
    "MaxDisccountPerLine" NUMERIC(12, 6),
    "MaxDisccountPerDocument" NUMERIC(12, 6),
    "SuperUser" BOOLEAN NOT NULL DEFAULT false,
    "IsLocked" BOOLEAN NOT NULL DEFAULT false,
    "IsActive" BOOLEAN NOT NULL DEFAULT true,
    "IsCanceled" BOOLEAN NOT NULL DEFAULT false,
    "Password" BYTEA,
    "ChangePasswordNextLogin" BOOLEAN NOT NULL DEFAULT false,
    "PasswordNeverExpires" BOOLEAN NOT NULL DEFAULT false,
    "CreatedBy" VARCHAR(50) NOT NULL DEFAULT 'system',
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "LastUpdatedBy" VARCHAR(50),
    "UpdateDate" TIMESTAMP
);

CREATE INDEX IF NOT EXISTS "IDX_Users_EmployeeCode" ON "Users" ("EmployeeCode");
CREATE INDEX IF NOT EXISTS "IDX_Users_RoleCode" ON "Users" ("RoleCode");



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
    "CreatedBy" VARCHAR(50) NOT NULL DEFAULT 'system',
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "LastUpdatedBy" VARCHAR(50),
    "UpdateDate" TIMESTAMP
);
CREATE INDEX IF NOT EXISTS "IDX_Employees_Code_Active" ON "Employees" ("Code") WHERE "IsCanceled" = false;
CREATE INDEX IF NOT EXISTS "IDX_Employees_Email" ON "Employees" ("Email");

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

-- 4. SystemConfigs
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
    "CreatedBy" VARCHAR(50) NOT NULL DEFAULT 'system',
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "LastUpdatedBy" VARCHAR(50),
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
    "CreatedBy" VARCHAR(50) NOT NULL DEFAULT 'system',
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "LastUpdatedBy" VARCHAR(50),
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
('StripeTransaction', 'STX', 0),
('TransferTransactions', 'TRF', 0),
('CashTransactions', 'CSH', 0),
('CardTransactions', 'CRD', 0),
('User', 'USR', 1),
('Employee', 'EMP', 0),
('Role', 'ROL', 1)
ON CONFLICT ("Domain") DO NOTHING;

-- 7. Venues
CREATE TABLE IF NOT EXISTS "Venues" (
    "Entry" SERIAL PRIMARY KEY,
    "Code" VARCHAR(50) NOT NULL UNIQUE,
    "Name" VARCHAR(255) NOT NULL,
    "City" VARCHAR(3) NOT NULL,
    "Address" TEXT NOT NULL,
    "IsActive" BOOLEAN NOT NULL DEFAULT true,
    "IsCanceled" BOOLEAN NOT NULL DEFAULT false,
    "CreatedBy" VARCHAR(50) NOT NULL DEFAULT 'system',
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "LastUpdatedBy" VARCHAR(50),
    "UpdateDate" TIMESTAMP,
    CONSTRAINT "CHK_Venues_IsActive" CHECK ("IsActive" IN (true, false))
);

CREATE UNIQUE INDEX IF NOT EXISTS "UQ_Venues_Name_Active" ON "Venues" (LOWER(TRIM("Name"))) WHERE "IsActive" = true AND "IsCanceled" = false;
CREATE INDEX IF NOT EXISTS "IDX_Venues_Code_Active" ON "Venues" ("Code") WHERE "IsActive" = true AND "IsCanceled" = false;

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
    "IsCanceled" BOOLEAN NOT NULL DEFAULT false,
    "CreatedBy" VARCHAR(50) NOT NULL DEFAULT 'system',
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "LastUpdatedBy" VARCHAR(50),
    "UpdateDate" TIMESTAMP,
    CONSTRAINT "CHK_Events_IsPromoted" CHECK ("IsPromoted" IN (true, false)),
    CONSTRAINT "CHK_Events_IsUpcoming" CHECK ("IsUpcoming" IN (true, false)),
    CONSTRAINT "CHK_Events_CommingSoon" CHECK ("CommingSoon" IN (true, false)),
    CONSTRAINT "CHK_Events_IsActive" CHECK ("IsActive" IN (true, false))
);

CREATE UNIQUE INDEX IF NOT EXISTS "UQ_Events_Name_Active" ON "Events" (LOWER(TRIM("Name"))) WHERE "IsActive" = true AND "IsCanceled" = false;
CREATE INDEX IF NOT EXISTS "IDX_Events_VenueCode_Active" ON "Events" ("VenueCode") WHERE "IsActive" = true AND "IsCanceled" = false;
CREATE INDEX IF NOT EXISTS "IDX_Events_Date_Active" ON "Events" ("Date") WHERE "IsActive" = true AND "IsCanceled" = false;
CREATE INDEX IF NOT EXISTS "IDX_Events_Category_Active" ON "Events" ("Category") WHERE "IsActive" = true AND "IsCanceled" = false;

-- 9. DeparturePoints
CREATE TABLE IF NOT EXISTS "DeparturePoints" (
    "Entry" SERIAL PRIMARY KEY,
    "Code" VARCHAR(50) NOT NULL UNIQUE,
    "Name" VARCHAR(255) NOT NULL,
    "City" VARCHAR(3) NOT NULL,
    "Address" TEXT NOT NULL,
    "PointType" "DeparturePointType" NOT NULL,
    "IsActive" BOOLEAN NOT NULL DEFAULT true,
    "IsCanceled" BOOLEAN NOT NULL DEFAULT false,
    "CreatedBy" VARCHAR(50) NOT NULL DEFAULT 'system',
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "LastUpdatedBy" VARCHAR(50),
    "UpdateDate" TIMESTAMP,
    CONSTRAINT "CHK_DeparturePoints_IsActive" CHECK ("IsActive" IN (true, false))
);

CREATE UNIQUE INDEX IF NOT EXISTS "UQ_DeparturePoints_Name_Active" ON "DeparturePoints" (LOWER(TRIM("Name"))) WHERE "IsActive" = true AND "IsCanceled" = false;
CREATE INDEX IF NOT EXISTS "IDX_DeparturePoints_City_PointType_Active" ON "DeparturePoints" ("City", "PointType") WHERE "IsActive" = true AND "IsCanceled" = false;

-- 10. Routes
CREATE TABLE IF NOT EXISTS "Routes" (
    "Entry" SERIAL PRIMARY KEY,
    "Code" VARCHAR(50) NOT NULL UNIQUE,
    "Name" VARCHAR(255) NOT NULL,
    "OriginPointCode" VARCHAR(50) NOT NULL,
    "DestinationVenueCode" VARCHAR(50) NOT NULL,
    "BasePrice" NUMERIC(10, 2) NOT NULL,
    "IsActive" BOOLEAN NOT NULL DEFAULT true,
    "IsCanceled" BOOLEAN NOT NULL DEFAULT false,
    "CreatedBy" VARCHAR(50) NOT NULL DEFAULT 'system',
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "LastUpdatedBy" VARCHAR(50),
    "UpdateDate" TIMESTAMP,
    CONSTRAINT "CHK_Routes_BasePrice" CHECK ("BasePrice" >= 0),
    CONSTRAINT "CHK_Routes_IsActive" CHECK ("IsActive" IN (true, false))
);

CREATE UNIQUE INDEX IF NOT EXISTS "UQ_Routes_Name_Active" ON "Routes" (LOWER(TRIM("Name"))) WHERE "IsActive" = true AND "IsCanceled" = false;
CREATE INDEX IF NOT EXISTS "IDX_Routes_Origin_Dest_Active" ON "Routes" ("OriginPointCode", "DestinationVenueCode") WHERE "IsActive" = true AND "IsCanceled" = false;

-- 11. RouteStops
CREATE TABLE IF NOT EXISTS "RouteStops" (
    "Entry" SERIAL PRIMARY KEY,
    "RouteCode" VARCHAR(50) NOT NULL,
    "DeparturePointCode" VARCHAR(50),
    "VenueCode" VARCHAR(50),
    "Type" "DeparturePointType" NOT NULL,
    "BasePrice" NUMERIC(10, 2) NOT NULL,
    "ArrivalTime" VARCHAR(50),
    "IsActive" BOOLEAN NOT NULL DEFAULT true,
    "IsCanceled" BOOLEAN NOT NULL DEFAULT false,
    "CreatedBy" VARCHAR(50) NOT NULL DEFAULT 'system',
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "LastUpdatedBy" VARCHAR(50),
    "UpdateDate" TIMESTAMP,
    CONSTRAINT "CHK_RouteStops_BasePrice" CHECK ("BasePrice" >= 0),
    CONSTRAINT "CHK_RouteStops_IsActive" CHECK ("IsActive" IN (true, false)),
    CONSTRAINT "CHK_RouteStops_Target" CHECK (
        ("Type" IN ('PICKUP', 'INTERMEDIATE') AND "DeparturePointCode" IS NOT NULL) OR
        ("Type" = 'DROPOFF' AND ("VenueCode" IS NOT NULL OR "DeparturePointCode" IS NOT NULL))
    )
);

CREATE INDEX IF NOT EXISTS "IDX_RouteStops_RouteCode_Active" ON "RouteStops" ("RouteCode") WHERE "IsActive" = true AND "IsCanceled" = false;
CREATE INDEX IF NOT EXISTS "IDX_RouteStops_DeparturePointCode" ON "RouteStops" ("DeparturePointCode") WHERE "IsActive" = true AND "IsCanceled" = false;
CREATE INDEX IF NOT EXISTS "IDX_RouteStops_VenueCode" ON "RouteStops" ("VenueCode") WHERE "IsActive" = true AND "IsCanceled" = false;
CREATE UNIQUE INDEX IF NOT EXISTS "UQ_RouteStops_RouteCode_Pickup_Active" ON "RouteStops" ("RouteCode") WHERE "Type" = 'PICKUP' AND "IsActive" = true AND "IsCanceled" = false;
CREATE UNIQUE INDEX IF NOT EXISTS "UQ_RouteStops_RouteCode_Dropoff_Active" ON "RouteStops" ("RouteCode") WHERE "Type" = 'DROPOFF' AND "IsActive" = true AND "IsCanceled" = false;

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
    "IsCanceled" BOOLEAN NOT NULL DEFAULT false,
    "CreatedBy" VARCHAR(50) NOT NULL DEFAULT 'system',
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "LastUpdatedBy" VARCHAR(50),
    "UpdateDate" TIMESTAMP,
    CONSTRAINT "CHK_Schedules_Capacity" CHECK ("Capacity" >= 0),
    CONSTRAINT "CHK_Schedules_AvailableSeats" CHECK ("AvailableSeats" >= 0),
    CONSTRAINT "CHK_Schedules_Price" CHECK ("Price" >= 0),
    CONSTRAINT "CHK_Schedules_AvailableSeats_Capacity" CHECK ("AvailableSeats" <= "Capacity"),
    CONSTRAINT "CHK_Schedules_IsActive" CHECK ("IsActive" IN (true, false))
);

CREATE INDEX IF NOT EXISTS "IDX_Schedules_EventCode_Type_Active" ON "Schedules" ("EventCode", "Type") WHERE "IsActive" = true AND "IsCanceled" = false;
CREATE INDEX IF NOT EXISTS "IDX_Schedules_RouteCode_Active" ON "Schedules" ("RouteCode") WHERE "IsActive" = true AND "IsCanceled" = false;

-- 13. Tickets
CREATE TABLE IF NOT EXISTS "Tickets" (
    "Entry" SERIAL PRIMARY KEY,
    "Code" VARCHAR(50) NOT NULL UNIQUE,
    "Name" VARCHAR(255) NOT NULL,
    "ScheduleCode" VARCHAR(50) NOT NULL,
    "Phone" VARCHAR(50) NOT NULL,
    "Quantity" INT NOT NULL DEFAULT 1,
    "IsActive" BOOLEAN NOT NULL DEFAULT true,
    "IsCanceled" BOOLEAN NOT NULL DEFAULT false,
    "CreatedBy" VARCHAR(50) NOT NULL DEFAULT 'system',
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "LastUpdatedBy" VARCHAR(50),
    "UpdateDate" TIMESTAMP,
    CONSTRAINT "CHK_Tickets_Phone" CHECK (LENGTH(TRIM("Phone")) >= 7),
    CONSTRAINT "CHK_Tickets_Quantity" CHECK ("Quantity" > 0),
    CONSTRAINT "CHK_Tickets_IsActive" CHECK ("IsActive" IN (true, false))
);

CREATE INDEX IF NOT EXISTS "IDX_Tickets_ScheduleCode_Active" ON "Tickets" ("ScheduleCode") WHERE "IsActive" = true AND "IsCanceled" = false;

-- 14. NotificationContacts
CREATE TABLE IF NOT EXISTS "NotificationContacts" (
    "Entry" SERIAL PRIMARY KEY,
    "Code" VARCHAR(50) NOT NULL UNIQUE,
    "Name" VARCHAR(255) NOT NULL,
    "Phone" VARCHAR(50) NOT NULL,
    "Role" VARCHAR(100) NOT NULL,
    "IsActive" BOOLEAN NOT NULL DEFAULT true,
    "IsCanceled" BOOLEAN NOT NULL DEFAULT false,
    "CreatedBy" VARCHAR(50) NOT NULL DEFAULT 'system',
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "LastUpdatedBy" VARCHAR(50),
    "UpdateDate" TIMESTAMP,
    CONSTRAINT "CHK_NotificationContacts_Phone" CHECK (LENGTH(TRIM("Phone")) >= 7),
    CONSTRAINT "CHK_NotificationContacts_IsActive" CHECK ("IsActive" IN (true, false))
);

CREATE UNIQUE INDEX IF NOT EXISTS "UQ_NotificationContacts_Phone_Active" ON "NotificationContacts" ("Phone") WHERE "IsActive" = true AND "IsCanceled" = false;

-- 15. NotificationSettings
CREATE TABLE IF NOT EXISTS "NotificationSettings" (
    "Entry" SERIAL PRIMARY KEY,
    "Code" VARCHAR(50) NOT NULL DEFAULT 'DEFAULT' UNIQUE,
    "Name" VARCHAR(255) NOT NULL DEFAULT 'Notification Settings',
    "OwnerWhatsapp" VARCHAR(50) NOT NULL,
    "IsCanceled" BOOLEAN NOT NULL DEFAULT false,
    "CreatedBy" VARCHAR(50) NOT NULL DEFAULT 'system',
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "LastUpdatedBy" VARCHAR(50),
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
    "PaymentMethod" VARCHAR(50) NOT NULL DEFAULT 'TRANSFER',
    "PaymentStatus" "PaymentStatusType" NOT NULL DEFAULT 'PENDING',
    "PaymentReference" VARCHAR(100),
    "PaymentDate" TIMESTAMP,
    "CancellationReason" TEXT,
    "CancelledDate" TIMESTAMP,
    "Status" "ReservationStatusType" NOT NULL DEFAULT 'UNCONFIRMED',
    "ExpiresAt" TIMESTAMPTZ,
    "IsActive" BOOLEAN NOT NULL DEFAULT true,
    "IsCanceled" BOOLEAN NOT NULL DEFAULT false,
    "CreatedBy" VARCHAR(50) NOT NULL DEFAULT 'system',
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "LastUpdatedBy" VARCHAR(50),
    "UpdateDate" TIMESTAMP,
    CONSTRAINT "CHK_Reservations_FirstName1" CHECK (LENGTH(TRIM("FirstName1")) > 0),
    CONSTRAINT "CHK_Reservations_LastName1" CHECK (LENGTH(TRIM("LastName1")) > 0),
    CONSTRAINT "CHK_Reservations_Phone" CHECK (LENGTH(TRIM("Phone")) >= 7),
    CONSTRAINT "CHK_Reservations_Seats" CHECK ("Seats" > 0),
    CONSTRAINT "CHK_Reservations_UnitPrice" CHECK ("UnitPrice" >= 0),
    CONSTRAINT "CHK_Reservations_TotalPrice" CHECK ("TotalPrice" >= 0),
    CONSTRAINT "CHK_Reservations_IsActive" CHECK ("IsActive" IN (true, false))
);

CREATE INDEX IF NOT EXISTS "IDX_Reservations_EventCode_Active" ON "Reservations" ("EventCode") WHERE "IsActive" = true AND "IsCanceled" = false;
CREATE INDEX IF NOT EXISTS "IDX_Reservations_RouteCode_Active" ON "Reservations" ("RouteCode") WHERE "IsActive" = true AND "IsCanceled" = false;
CREATE INDEX IF NOT EXISTS "IDX_Reservations_Phone_Active" ON "Reservations" ("Phone") WHERE "IsActive" = true AND "IsCanceled" = false;
CREATE INDEX IF NOT EXISTS "IDX_Reservations_Status_Active" ON "Reservations" ("Status") WHERE "IsActive" = true AND "IsCanceled" = false;
CREATE INDEX IF NOT EXISTS "IDX_Reservations_PaymentStatus_Active" ON "Reservations" ("PaymentStatus") WHERE "IsActive" = true AND "IsCanceled" = false;
CREATE INDEX IF NOT EXISTS "IDX_Reservations_PaymentMethod_Active" ON "Reservations" ("PaymentMethod") WHERE "IsActive" = true AND "IsCanceled" = false;
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
    "WhatsAppTemplateCode" VARCHAR(100),
    "IsActive" BOOLEAN NOT NULL DEFAULT true,
    "IsCanceled" BOOLEAN NOT NULL DEFAULT false,
    "CreatedBy" VARCHAR(50) NOT NULL DEFAULT 'system',
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "LastUpdatedBy" VARCHAR(50),
    "UpdateDate" TIMESTAMP,
    CONSTRAINT "CHK_PaymentMethods_IsActive" CHECK ("IsActive" IN (true, false))
);

CREATE UNIQUE INDEX IF NOT EXISTS "UQ_PaymentMethods_Name_Active" ON "PaymentMethods" (LOWER(TRIM("Name"))) WHERE "IsActive" = true AND "IsCanceled" = false;

INSERT INTO "PaymentMethods" ("Code", "Name", "Type", "UsesCommission", "CommissionRate", "CreatedBy") VALUES
('PMT1', 'Efectivo', 'CASH', false, 0.00, 'system'),
('PMT2', 'Transferencia', 'TRANSFER', false, 0.00, 'system'),
('PMT3', 'Tarjeta / Otro', 'CARD', true, 3.50, 'system'),
('PMT4', 'No Aplica / Pend.', 'NOT_APPLICABLE', false, 0.00, 'system')
ON CONFLICT ("Code") DO NOTHING;

-- 17.1 PaymentMethodDetails (Detalles bancarios para transferencias/sin integración)
CREATE TABLE IF NOT EXISTS "PaymentMethodDetails" (
    "Entry"            SERIAL PRIMARY KEY,
    "Code"             VARCHAR(50) NOT NULL,
    "BankName"         VARCHAR(255) NOT NULL,
    "CLABE"            VARCHAR(255) NOT NULL,
    "AccountHolder"    VARCHAR(255) NOT NULL,
    "PaymentReference" VARCHAR(255),
    "IsActive"         BOOLEAN NOT NULL DEFAULT true,
    "CreatedBy"        VARCHAR(50) NOT NULL DEFAULT 'system',
    "CreateDate"       TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "LastUpdatedBy"    VARCHAR(50),
    "UpdateDate"       TIMESTAMP,
    CONSTRAINT "CHK_PaymentMethodDetails_IsActive" CHECK ("IsActive" IN (true, false))
);

CREATE INDEX IF NOT EXISTS "IDX_PMDetails_Code" 
    ON "PaymentMethodDetails" ("Code");
CREATE INDEX IF NOT EXISTS "IDX_PMDetails_Active" 
    ON "PaymentMethodDetails" ("Code", "IsActive") WHERE "IsActive" = true;

-- 18. PaymentTransactions (Auditoría Universal de Transacciones de Cobro)
CREATE TABLE IF NOT EXISTS "PaymentTransactions" (
    "Entry" SERIAL PRIMARY KEY,
    "Code" VARCHAR(50) NOT NULL UNIQUE,
    "EntityType" VARCHAR(50) NOT NULL DEFAULT 'ROUTE_RESERVATION',
    "EntityCode" VARCHAR(50) NOT NULL,
    "Provider" VARCHAR(50) NOT NULL DEFAULT 'STRIPE',
    "PaymentMethod" VARCHAR(50) NOT NULL DEFAULT 'CARD',
    "Amount" NUMERIC(10, 2) NOT NULL,
    "Currency" VARCHAR(10) NOT NULL DEFAULT 'mxn',
    "Status" VARCHAR(50) NOT NULL DEFAULT 'PENDING',
    "PaymentIntentId" VARCHAR(255),
    "SessionId" VARCHAR(255),
    "Clabe" VARCHAR(50),
    "BankName" VARCHAR(100),
    "ReceiptUrl" TEXT,
    "HostedUrl" TEXT,
    "RawPayload" TEXT,
    "ExpiresAt" TIMESTAMPTZ,
    "IsActive" BOOLEAN NOT NULL DEFAULT true,
    "CreatedBy" VARCHAR(50) NOT NULL DEFAULT 'system',
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "LastUpdatedBy" VARCHAR(50),
    "UpdateDate" TIMESTAMP
);

CREATE INDEX IF NOT EXISTS "IDX_PaymentTransactions_Entity" ON "PaymentTransactions" ("EntityType", "EntityCode");
CREATE INDEX IF NOT EXISTS "IDX_PaymentTransactions_Status" ON "PaymentTransactions" ("Status");
CREATE INDEX IF NOT EXISTS "IDX_PaymentTransactions_ExpiresAt_Pending" ON "PaymentTransactions" ("ExpiresAt") WHERE "Status" = 'PENDING' AND "IsActive" = true;
CREATE INDEX IF NOT EXISTS "IDX_PaymentTransactions_PaymentIntentId" ON "PaymentTransactions" ("PaymentIntentId");
CREATE INDEX IF NOT EXISTS "IDX_PaymentTransactions_Clabe" ON "PaymentTransactions" ("Clabe");

-- 18.1 Integrations (Platform Master Catalog of Integrations)
CREATE TABLE IF NOT EXISTS "Integrations" (
    "Entry" SERIAL PRIMARY KEY,
    "Code" VARCHAR(50) NOT NULL UNIQUE,
    "Name" VARCHAR(100) NOT NULL,
    "DisplayName" VARCHAR(255) NOT NULL,
    "Category" VARCHAR(50) NOT NULL DEFAULT 'PAYMENT',
    "SettingsTable" VARCHAR(100),
    "LicenseModule" VARCHAR(50),
    "RequiredColumns" VARCHAR(255),
    "Description" TEXT,
    "IsActive" BOOLEAN NOT NULL DEFAULT true,
    "CreatedBy" VARCHAR(50) NOT NULL DEFAULT 'system',
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "LastUpdatedBy" VARCHAR(50),
    "UpdateDate" TIMESTAMP
);

INSERT INTO "Integrations" ("Code", "Name", "DisplayName", "Category", "SettingsTable", "LicenseModule", "RequiredColumns", "Description", "IsActive")
VALUES
    ('STRIPE', 'Stripe', 'Stripe Payments (Tarjeta / SPEI)', 'PAYMENT', 'StripeSettings', 'MODULE_STRIPE', 'PublishableKey,SecretKey', 'Procesamiento de pagos con tarjeta de crédito/débito y transferencias bancarias SPEI.', true),
    ('MERCADOPAGO', 'Mercado Pago', 'Mercado Pago (Tarjeta / Wallet)', 'PAYMENT', 'MercadoPagoSettings', 'MODULE_MERCADOPAGO', 'PublicKey,AccessToken', 'Plataforma de pagos y billetera virtual en América Latina.', true),
    ('OPENPAY', 'OpenPay', 'OpenPay BBVA (Tarjeta / Paynet)', 'PAYMENT', 'OpenPaySettings', 'MODULE_OPENPAY', 'MerchantId,PublicKey,PrivateKey', 'Pasarela de pagos respaldada por BBVA México con soporte para tarjetas y tiendas Paynet.', true),
    ('WHATSAPP', 'WhatsApp', 'WhatsApp Meta Cloud API', 'MESSAGING', 'WhatsAppSettings', 'MODULE_WHATSAPP', 'PhoneId,ApiToken', 'Envío y recepción de notificaciones a través de la API oficial de WhatsApp Cloud.', true),
    ('TICKETMASTER', 'Ticketmaster', 'Ticketmaster Discovery API v2', 'EVENTS', 'TicketmasterSettings', 'MODULE_EVENTS', 'ApiKey,ApiBaseUrl', 'Descubrimiento y sincronización automatizada de eventos, recintos (venues) y destinos.', true)
ON CONFLICT ("Code") DO UPDATE SET
    "Name" = EXCLUDED."Name",
    "DisplayName" = EXCLUDED."DisplayName",
    "Category" = EXCLUDED."Category",
    "SettingsTable" = EXCLUDED."SettingsTable",
    "LicenseModule" = EXCLUDED."LicenseModule",
    "RequiredColumns" = EXCLUDED."RequiredColumns",
    "Description" = EXCLUDED."Description";

-- 18.2 TicketmasterSettings
CREATE TABLE IF NOT EXISTS "TicketmasterSettings" (
    "Entry" SERIAL PRIMARY KEY,
    "Code" VARCHAR(50) NOT NULL DEFAULT 'DEFAULT' UNIQUE,
    "Name" VARCHAR(255) NOT NULL DEFAULT 'Ticketmaster Discovery Settings',
    "ApiKey" TEXT,
    "ApiSecret" TEXT,
    "ApiBaseUrl" VARCHAR(255) NOT NULL DEFAULT 'https://app.ticketmaster.com',
    "ApiVersion" VARCHAR(10) NOT NULL DEFAULT 'v2',
    "DefaultCountryCode" VARCHAR(5) NOT NULL DEFAULT 'MX',
    "DefaultCity" VARCHAR(100),
    "DefaultClassification" VARCHAR(100) DEFAULT 'Music',
    "AutoCreateRoutes" BOOLEAN NOT NULL DEFAULT false,
    "DefaultOriginPointCode" VARCHAR(50) DEFAULT 'DEP1',
    "SyncIntervalMinutes" INT NOT NULL DEFAULT 1440,
    "IsActive" BOOLEAN NOT NULL DEFAULT true,
    "LastSyncDate" TIMESTAMP,
    "CreatedBy" VARCHAR(50) NOT NULL DEFAULT 'system',
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "LastUpdatedBy" VARCHAR(50),
    "UpdateDate" TIMESTAMP
);

-- 19. StripeSettings
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
    "IsActive" BOOLEAN NOT NULL DEFAULT false,
    "CreatedBy" VARCHAR(50) NOT NULL DEFAULT 'system',
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "LastUpdatedBy" VARCHAR(50),
    "UpdateDate" TIMESTAMP
);

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
    "CreatedBy" VARCHAR(50) NOT NULL DEFAULT 'system',
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "LastUpdatedBy" VARCHAR(50),
    "UpdateDate" TIMESTAMP
);

CREATE INDEX IF NOT EXISTS "IDX_StripeTransactions_ReservationCode" ON "StripeTransactions" ("ReservationCode");
CREATE INDEX IF NOT EXISTS "IDX_StripeTransactions_PaymentIntentId" ON "StripeTransactions" ("PaymentIntentId");
CREATE INDEX IF NOT EXISTS "IDX_StripeTransactions_Clabe" ON "StripeTransactions" ("Clabe");

-- 24.1 MercadoPagoSettings
CREATE TABLE IF NOT EXISTS "MercadoPagoSettings" (
    "Entry" SERIAL PRIMARY KEY,
    "Code" VARCHAR(50) NOT NULL DEFAULT 'DEFAULT' UNIQUE,
    "Name" VARCHAR(255) NOT NULL DEFAULT 'Mercado Pago Settings',
    "PublicKey" TEXT,
    "AccessToken" TEXT,
    "WebhookSecretKey" TEXT,
    "ApiBaseUrl" VARCHAR(255) NOT NULL DEFAULT 'https://api.mercadopago.com',
    "WebhookPath" VARCHAR(255) NOT NULL DEFAULT '/api/v1/mercadopago/webhook',
    "Currency" VARCHAR(10) NOT NULL DEFAULT 'mxn',
    "IsTestMode" BOOLEAN NOT NULL DEFAULT true,
    "IsActive" BOOLEAN NOT NULL DEFAULT false,
    "CreatedBy" VARCHAR(50) NOT NULL DEFAULT 'system',
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "LastUpdatedBy" VARCHAR(50),
    "UpdateDate" TIMESTAMP
);

-- 24.2 MercadoPagoTransactions (Auditing & Webhook Correlation)
CREATE TABLE IF NOT EXISTS "MercadoPagoTransactions" (
    "Entry" SERIAL PRIMARY KEY,
    "Code" VARCHAR(50) NOT NULL UNIQUE,
    "ExternalReference" VARCHAR(50) NOT NULL,
    "PaymentId" VARCHAR(255) NOT NULL,
    "PreferenceId" VARCHAR(255),
    "Amount" NUMERIC(10, 2) NOT NULL,
    "Currency" VARCHAR(10) NOT NULL DEFAULT 'mxn',
    "Status" VARCHAR(50) NOT NULL DEFAULT 'pending',
    "StatusDetail" VARCHAR(255),
    "PaymentMethodId" VARCHAR(50),
    "PaymentTypeId" VARCHAR(50),
    "Installments" INT DEFAULT 1,
    "TicketUrl" TEXT,
    "Clabe" VARCHAR(50),
    "CardBrand" VARCHAR(50),
    "CardLast4" VARCHAR(10),
    "ClientIp" VARCHAR(50),
    "IsActive" BOOLEAN NOT NULL DEFAULT true,
    "CreatedBy" VARCHAR(50) NOT NULL DEFAULT 'system',
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "LastUpdatedBy" VARCHAR(50),
    "UpdateDate" TIMESTAMP
);

CREATE INDEX IF NOT EXISTS "IDX_MercadoPagoTransactions_ExternalReference" ON "MercadoPagoTransactions" ("ExternalReference");
CREATE INDEX IF NOT EXISTS "IDX_MercadoPagoTransactions_PaymentId" ON "MercadoPagoTransactions" ("PaymentId");

-- 24.3 OpenPaySettings (Configuración Nativa de OpenPay)
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
    "CreatedBy" VARCHAR(50) NOT NULL DEFAULT 'system',
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "LastUpdatedBy" VARCHAR(50),
    "UpdateDate" TIMESTAMP
);

INSERT INTO "OpenPaySettings" ("Code", "Name", "IsTestMode", "IsActive", "CreatedBy") VALUES
('DEFAULT', 'OpenPay Settings', true, true, 'system')
ON CONFLICT ("Code") DO NOTHING;

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
    "CreatedBy" VARCHAR(50) NOT NULL DEFAULT 'system',
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "LastUpdatedBy" VARCHAR(50),
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
    "CreatedBy" VARCHAR(50) NOT NULL DEFAULT 'system',
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "ContextReservationCode" VARCHAR(50),
    "LastUpdatedBy" VARCHAR(50),
    "UpdateDate" TIMESTAMP
);

CREATE INDEX IF NOT EXISTS "IDX_WhatsAppConversations_CustomerPhone" ON "WhatsAppConversations" ("CustomerPhone");
CREATE INDEX IF NOT EXISTS "IDX_WhatsAppConversations_ContextReservation" ON "WhatsAppConversations" ("ContextReservationCode");

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
    "MediaId" VARCHAR(100),
    "MediaMimeType" VARCHAR(100),
    "MediaFileName" VARCHAR(255),
    "ContextWhatsAppId" VARCHAR(255),
    "MessageTimestamp" TIMESTAMP,
    "DeliveredDate" TIMESTAMP,
    "ReadDate" TIMESTAMP,
    "Status" VARCHAR(50) NOT NULL DEFAULT 'SENT',
    "ResponsePayload" TEXT,
    "ErrorMessage" TEXT,
    "SentBy" INT NOT NULL DEFAULT 1,
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP
);

CREATE INDEX IF NOT EXISTS "IDX_WhatsAppMessages_ConversationEntry" ON "WhatsAppMessages" ("ConversationEntry");
CREATE INDEX IF NOT EXISTS "IDX_WhatsAppMessages_Code" ON "WhatsAppMessages" ("Code");
CREATE INDEX IF NOT EXISTS "IDX_WhatsAppMessages_WhatsAppId" ON "WhatsAppMessages" ("WhatsAppId");
CREATE INDEX IF NOT EXISTS "IDX_WhatsAppMessages_ContextWhatsAppId" ON "WhatsAppMessages" ("ContextWhatsAppId");
CREATE INDEX IF NOT EXISTS "IDX_WhatsAppMessages_MessageTimestamp" ON "WhatsAppMessages" ("MessageTimestamp");

-- =============================================================================
-- 28. WhatsApp Templates Modular Architecture (Meta Cloud API)
-- =============================================================================

-- 28.1 WhatsAppTemplates (Master)
CREATE TABLE IF NOT EXISTS "WhatsAppTemplates" (
    "Entry" SERIAL PRIMARY KEY,
    "Code" VARCHAR(50) NOT NULL UNIQUE,
    "TemplateName" VARCHAR(100) NOT NULL,
    "Language" VARCHAR(10) NOT NULL DEFAULT 'es_MX',
    "Category" VARCHAR(50) NOT NULL DEFAULT 'UTILITY',
    "MetaTemplateId" VARCHAR(100),
    "Status" VARCHAR(50) NOT NULL DEFAULT 'PENDING',
    "RejectReason" TEXT,
    "IsActive" BOOLEAN NOT NULL DEFAULT true,
    "CreatedBy" VARCHAR(50) NOT NULL DEFAULT 'system',
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "LastUpdatedBy" VARCHAR(50),
    "UpdateDate" TIMESTAMP
);

CREATE INDEX IF NOT EXISTS "IDX_WhatsAppTemplates_TemplateName" ON "WhatsAppTemplates" ("TemplateName");
CREATE INDEX IF NOT EXISTS "IDX_WhatsAppTemplates_Code" ON "WhatsAppTemplates" ("Code");
CREATE INDEX IF NOT EXISTS "IDX_WhatsAppTemplates_MetaTemplateId" ON "WhatsAppTemplates" ("MetaTemplateId");

-- 28.2 WhatsAppTemplateHeaders (1:1 por plantilla)
CREATE TABLE IF NOT EXISTS "WhatsAppTemplateHeaders" (
    "Entry" SERIAL PRIMARY KEY,
    "TemplateCode" VARCHAR(50) NOT NULL,
    "HeaderType" VARCHAR(20) NOT NULL DEFAULT 'NONE', -- 'NONE', 'TEXT', 'IMAGE', 'DOCUMENT', 'VIDEO'
    "TextContent" TEXT,
    "MediaUrl" TEXT,
    "CreatedBy" VARCHAR(50) NOT NULL DEFAULT 'system',
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    CONSTRAINT "UQ_TemplateHeader_TemplateCode" UNIQUE ("TemplateCode")
);

CREATE INDEX IF NOT EXISTS "IDX_WhatsAppTemplateHeaders_TemplateCode" ON "WhatsAppTemplateHeaders" ("TemplateCode");

-- 28.3 WhatsAppTemplateHeaderVariables (1:N variables del Header)
CREATE TABLE IF NOT EXISTS "WhatsAppTemplateHeaderVariables" (
    "Entry" SERIAL PRIMARY KEY,
    "TemplateCode" VARCHAR(50) NOT NULL,
    "VarName" VARCHAR(50) NOT NULL,
    "VarIndex" INT NOT NULL DEFAULT 1,
    "ExampleValue" VARCHAR(255),
    "CreatedBy" VARCHAR(50) NOT NULL DEFAULT 'system',
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    CONSTRAINT "UQ_TemplateHeaderVar" UNIQUE ("TemplateCode", "VarIndex")
);

CREATE INDEX IF NOT EXISTS "IDX_WhatsAppTemplateHeaderVars_TemplateCode" ON "WhatsAppTemplateHeaderVariables" ("TemplateCode");

-- 28.4 WhatsAppTemplateBodies (1:1 por plantilla)
CREATE TABLE IF NOT EXISTS "WhatsAppTemplateBodies" (
    "Entry" SERIAL PRIMARY KEY,
    "TemplateCode" VARCHAR(50) NOT NULL,
    "BodyText" TEXT NOT NULL,
    "CreatedBy" VARCHAR(50) NOT NULL DEFAULT 'system',
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    CONSTRAINT "UQ_TemplateBody_TemplateCode" UNIQUE ("TemplateCode")
);

CREATE INDEX IF NOT EXISTS "IDX_WhatsAppTemplateBodies_TemplateCode" ON "WhatsAppTemplateBodies" ("TemplateCode");

-- 28.5 WhatsAppTemplateBodyVariables (1:N variables del Body)
CREATE TABLE IF NOT EXISTS "WhatsAppTemplateBodyVariables" (
    "Entry" SERIAL PRIMARY KEY,
    "TemplateCode" VARCHAR(50) NOT NULL,
    "VarName" VARCHAR(50) NOT NULL,
    "VarIndex" INT NOT NULL DEFAULT 1,
    "ExampleValue" VARCHAR(255),
    "CreatedBy" VARCHAR(50) NOT NULL DEFAULT 'system',
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    CONSTRAINT "UQ_TemplateBodyVar" UNIQUE ("TemplateCode", "VarIndex")
);

CREATE INDEX IF NOT EXISTS "IDX_WhatsAppTemplateBodyVars_TemplateCode" ON "WhatsAppTemplateBodyVariables" ("TemplateCode");

-- 28.6 WhatsAppTemplateFooters (1:1 por plantilla)
CREATE TABLE IF NOT EXISTS "WhatsAppTemplateFooters" (
    "Entry" SERIAL PRIMARY KEY,
    "TemplateCode" VARCHAR(50) NOT NULL,
    "FooterText" TEXT,
    "CreatedBy" VARCHAR(50) NOT NULL DEFAULT 'system',
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    CONSTRAINT "UQ_TemplateFooter_TemplateCode" UNIQUE ("TemplateCode")
);

CREATE INDEX IF NOT EXISTS "IDX_WhatsAppTemplateFooters_TemplateCode" ON "WhatsAppTemplateFooters" ("TemplateCode");

-- 28.7 WhatsAppTemplateFooterVariables (1:N variables del Footer si aplica)
CREATE TABLE IF NOT EXISTS "WhatsAppTemplateFooterVariables" (
    "Entry" SERIAL PRIMARY KEY,
    "TemplateCode" VARCHAR(50) NOT NULL,
    "VarName" VARCHAR(50) NOT NULL,
    "VarIndex" INT NOT NULL DEFAULT 1,
    "ExampleValue" VARCHAR(255),
    "CreatedBy" VARCHAR(50) NOT NULL DEFAULT 'system',
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    CONSTRAINT "UQ_TemplateFooterVar" UNIQUE ("TemplateCode", "VarIndex")
);

CREATE INDEX IF NOT EXISTS "IDX_WhatsAppTemplateFooterVars_TemplateCode" ON "WhatsAppTemplateFooterVariables" ("TemplateCode");

-- 28.8 WhatsAppTemplateButtons (1:N botones interactivos de la plantilla)
CREATE TABLE IF NOT EXISTS "WhatsAppTemplateButtons" (
    "Entry" SERIAL PRIMARY KEY,
    "TemplateCode" VARCHAR(50) NOT NULL,
    "ButtonType" VARCHAR(30) NOT NULL DEFAULT 'QUICK_REPLY', -- 'QUICK_REPLY', 'URL', 'PHONE_NUMBER'
    "Title" VARCHAR(100) NOT NULL,
    "Payload" TEXT, -- URL, Teléfono o ID
    "SortOrder" INT NOT NULL DEFAULT 1,
    "CreatedBy" VARCHAR(50) NOT NULL DEFAULT 'system',
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP
);

CREATE INDEX IF NOT EXISTS "IDX_WhatsAppTemplateButtons_TemplateCode" ON "WhatsAppTemplateButtons" ("TemplateCode");

-- 28.9 WhatsAppInternalTemplates
CREATE TABLE IF NOT EXISTS "WhatsAppInternalTemplates" (
    "Entry" SERIAL PRIMARY KEY,
    "TemplateCode" VARCHAR(50) NOT NULL,
    "ReceiptReceivedType" VARCHAR(30) NOT NULL DEFAULT 'TEXT',
    "ReceiptReceivedPayload" TEXT NOT NULL DEFAULT '',
    "InvalidFileType" VARCHAR(30) NOT NULL DEFAULT 'TEXT',
    "InvalidFilePayload" TEXT NOT NULL DEFAULT '',
    "ReceiptAcceptedType" VARCHAR(30) NOT NULL DEFAULT 'TEXT',
    "ReceiptAcceptedPayload" TEXT NOT NULL DEFAULT '',
    "ReceiptRejectedType" VARCHAR(30) NOT NULL DEFAULT 'TEXT',
    "ReceiptRejectedPayload" TEXT NOT NULL DEFAULT '',
    "ReceiptReceivedMessage" TEXT NOT NULL DEFAULT '',
    "InvalidFileMessage" TEXT NOT NULL DEFAULT '',
    "ReceiptAcceptedMessage" TEXT NOT NULL DEFAULT '',
    "ReceiptRejectedMessage" TEXT NOT NULL DEFAULT '',
    "ConfirmationTemplateCode" VARCHAR(100) NOT NULL DEFAULT '',
    "IsActive" BOOLEAN NOT NULL DEFAULT true,
    "CreatedBy" VARCHAR(50) NOT NULL DEFAULT 'system',
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "LastUpdatedBy" VARCHAR(50),
    "UpdateDate" TIMESTAMP,
    CONSTRAINT "UQ_WhatsAppInternalTemplates_TemplateCode" UNIQUE ("TemplateCode")
);

CREATE INDEX IF NOT EXISTS "IDX_WhatsAppInternalTemplates_TemplateCode" ON "WhatsAppInternalTemplates" ("TemplateCode");

-- 28.10 WhatsAppInternalButtonActions
CREATE TABLE IF NOT EXISTS "WhatsAppInternalButtonActions" (
    "Entry" SERIAL PRIMARY KEY,
    "TemplateCode" VARCHAR(50) NOT NULL,
    "ButtonTitle" VARCHAR(100) NOT NULL,
    "SortOrder" INT NOT NULL DEFAULT 1,
    "MessageType" VARCHAR(30) NOT NULL DEFAULT 'NONE',
    "ResponsePayload" TEXT NOT NULL DEFAULT '',
    "TriggerAction" VARCHAR(50) NOT NULL DEFAULT 'NONE',
    "ResponseMessage" TEXT NOT NULL DEFAULT '',
    "CreatedBy" VARCHAR(50) NOT NULL DEFAULT 'system',
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    CONSTRAINT "UQ_WhatsAppInternalBtn_Template_Title" UNIQUE ("TemplateCode", "ButtonTitle")
);

CREATE INDEX IF NOT EXISTS "IDX_WhatsAppInternalBtn_TemplateCode" ON "WhatsAppInternalButtonActions" ("TemplateCode");

-- Migration / Cleanup of legacy CustomMessages tables
DO $$
BEGIN
    DROP TABLE IF EXISTS "CustomAttachments" CASCADE;
    DROP TABLE IF EXISTS "CustomButtons" CASCADE;
    DROP TABLE IF EXISTS "CustomMessageParameters" CASCADE;
    DROP TABLE IF EXISTS "CustomMessages" CASCADE;
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
    "CreatedBy" VARCHAR(50) NOT NULL DEFAULT 'system',
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
    "CreatedBy" VARCHAR(50) NOT NULL DEFAULT 'system',
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "LastUpdatedBy" VARCHAR(50),
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
    "CreatedBy" VARCHAR(50) NOT NULL DEFAULT 'system',
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "LastUpdatedBy" VARCHAR(50),
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
    "CreatedBy" VARCHAR(50) NOT NULL DEFAULT 'system',
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "LastUpdatedBy" VARCHAR(50),
    "UpdateDate" TIMESTAMP
);

-- D1. RoleModules
CREATE TABLE IF NOT EXISTS "RoleModules" (
    "Entry" SERIAL PRIMARY KEY,
    "RoleCode" VARCHAR(50) NOT NULL,
    "ModuleCode" VARCHAR(50) NOT NULL,
    "IsAllowed" BOOLEAN NOT NULL DEFAULT true,
    "AllowOverride" BOOLEAN NOT NULL DEFAULT false,
    "IsActive" BOOLEAN NOT NULL DEFAULT true,
    "CreatedBy" VARCHAR(50) NOT NULL DEFAULT 'system',
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "LastUpdatedBy" VARCHAR(50),
    "UpdateDate" TIMESTAMP,
    CONSTRAINT "UQ_RoleModules" UNIQUE ("RoleCode", "ModuleCode")
);
CREATE INDEX IF NOT EXISTS "IDX_RoleModules_RoleCode" ON "RoleModules" ("RoleCode");
CREATE INDEX IF NOT EXISTS "IDX_RoleModules_ModuleCode" ON "RoleModules" ("ModuleCode");

-- D2. RolePermissions
CREATE TABLE IF NOT EXISTS "RolePermissions" (
    "Entry" SERIAL PRIMARY KEY,
    "RoleCode" VARCHAR(50) NOT NULL,
    "PermissionCode" VARCHAR(50) NOT NULL,
    "ModuleCode" VARCHAR(50) NOT NULL,
    "IsAllowed" BOOLEAN NOT NULL DEFAULT true,
    "AllowOverride" BOOLEAN NOT NULL DEFAULT false,
    "IsActive" BOOLEAN NOT NULL DEFAULT true,
    "CreatedBy" VARCHAR(50) NOT NULL DEFAULT 'system',
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "LastUpdatedBy" VARCHAR(50),
    "UpdateDate" TIMESTAMP,
    CONSTRAINT "UQ_RolePermissions" UNIQUE ("RoleCode", "PermissionCode")
);
CREATE INDEX IF NOT EXISTS "IDX_RolePermissions_RoleCode" ON "RolePermissions" ("RoleCode");

-- E1. UserModules
CREATE TABLE IF NOT EXISTS "UserModules" (
    "Entry" SERIAL PRIMARY KEY,
    "UserCode" VARCHAR(50) NOT NULL,
    "ModuleCode" VARCHAR(50) NOT NULL,
    "IsAllowed" BOOLEAN NOT NULL DEFAULT true,
    "AllowOverride" BOOLEAN NOT NULL DEFAULT false,
    "GrantedByCode" VARCHAR(50),
    "IsActive" BOOLEAN NOT NULL DEFAULT true,
    "CreatedBy" VARCHAR(50) NOT NULL DEFAULT 'system',
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "LastUpdatedBy" VARCHAR(50),
    "UpdateDate" TIMESTAMP,
    CONSTRAINT "UQ_UserModules" UNIQUE ("UserCode", "ModuleCode")
);
CREATE INDEX IF NOT EXISTS "IDX_UserModules_UserCode" ON "UserModules" ("UserCode");
CREATE INDEX IF NOT EXISTS "IDX_UserModules_ModuleCode" ON "UserModules" ("ModuleCode");

-- E2. UserPermissions
CREATE TABLE IF NOT EXISTS "UserPermissions" (
    "Entry" SERIAL PRIMARY KEY,
    "UserCode" VARCHAR(50) NOT NULL,
    "PermissionCode" VARCHAR(50) NOT NULL,
    "ModuleCode" VARCHAR(50) NOT NULL,
    "IsAllowed" BOOLEAN NOT NULL DEFAULT true,
    "AllowOverride" BOOLEAN NOT NULL DEFAULT false,
    "GrantedByCode" VARCHAR(50),
    "IsActive" BOOLEAN NOT NULL DEFAULT true,
    "CreatedBy" VARCHAR(50) NOT NULL DEFAULT 'system',
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "LastUpdatedBy" VARCHAR(50),
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

-- Cleanup obsolete/deprecated modules and permissions from database
DELETE FROM "Permissions" WHERE "ModuleCode" IN ('MOD_CATALOGS', 'MOD_CUSTOMERS') OR "Code" IN ('ROUTE_CATALOG_CREATE', 'ROUTE_CATALOG_UPDATE', 'ROUTE_CATALOG_DELETE', 'CORE_CUSTOMER_CREATE', 'CORE_CUSTOMER_UPDATE', 'CORE_CUSTOMER_DELETE');
DELETE FROM "UserPermissions" WHERE "PermissionCode" IN ('ROUTE_CATALOG_CREATE', 'ROUTE_CATALOG_UPDATE', 'ROUTE_CATALOG_DELETE', 'CORE_CUSTOMER_CREATE', 'CORE_CUSTOMER_UPDATE', 'CORE_CUSTOMER_DELETE');
DELETE FROM "RolePermissions" WHERE "PermissionCode" IN ('ROUTE_CATALOG_CREATE', 'ROUTE_CATALOG_UPDATE', 'ROUTE_CATALOG_DELETE', 'CORE_CUSTOMER_CREATE', 'CORE_CUSTOMER_UPDATE', 'CORE_CUSTOMER_DELETE');
DELETE FROM "RoleModules" WHERE "ModuleCode" IN ('MOD_CATALOGS', 'MOD_CUSTOMERS', 'MOD_DASHBOARD');
DELETE FROM "UserModules" WHERE "ModuleCode" IN ('MOD_CATALOGS', 'MOD_CUSTOMERS', 'MOD_DASHBOARD');
DELETE FROM "Modules" WHERE "Code" IN ('MOD_CATALOGS', 'MOD_CUSTOMERS', 'MOD_DASHBOARD');

-- Corregir cabeceras de Rutas que tengan OriginPointCode o DestinationVenueCode vacíos
UPDATE "Routes" r
SET "OriginPointCode" = COALESCE(
  NULLIF(r."OriginPointCode", ''),
  (SELECT rs."DeparturePointCode" FROM "RouteStops" rs WHERE rs."RouteCode" = r."Code" AND (rs."Type" = 'PICKUP' OR rs."Type" = 'origin') AND rs."DeparturePointCode" IS NOT NULL AND rs."DeparturePointCode" != '' LIMIT 1)
),
"DestinationVenueCode" = COALESCE(
  NULLIF(r."DestinationVenueCode", ''),
  (SELECT rs."VenueCode" FROM "RouteStops" rs WHERE rs."RouteCode" = r."Code" AND (rs."Type" = 'DROPOFF' OR rs."Type" = 'destination') AND rs."VenueCode" IS NOT NULL AND rs."VenueCode" != '' LIMIT 1)
)
WHERE (r."OriginPointCode" IS NULL OR r."OriginPointCode" = '')
   OR (r."DestinationVenueCode" IS NULL OR r."DestinationVenueCode" = '');

-- Seed Modules (17 Official Modules)
INSERT INTO "Modules" ("Code", "Name", "Description", "Icon", "SortOrder", "IsActive", "CreatedBy") VALUES
('MOD_OVERVIEW', 'Dashboard Analítico', 'Métricas, KPIs y estadísticas globales', 'dashboard', 1, true, 'system'),
('MOD_RESERVATIONS', 'Reservaciones', 'Gestión de boletos, pasajeros y cobros', 'confirmation_number', 2, true, 'system'),
('MOD_SCHEDULES', 'Horarios y Salidas', 'Programación de corridas y viajes', 'schedule', 3, true, 'system'),
('MOD_VENUES', 'Destinos', 'Gestión de destinos, sedes y recintos', 'flag', 4, true, 'system'),
('MOD_POINTS', 'Orígenes y Paradas', 'Puntos de abordaje y paradas intermedias', 'place', 5, true, 'system'),
('MOD_ROUTES', 'Rutas de Viaje', 'Trazado de rutas y tarifas base', 'alt_route', 6, true, 'system'),
('MOD_EVENTS', 'Eventos', 'Conciertos, festivales y eventos', 'event', 7, true, 'system'),
('MOD_SETTINGS', 'Configuración General', 'Parámetros del sistema y políticas', 'settings', 8, true, 'system'),
('MOD_EMPLOYEES', 'Empleados y Personal', 'Gestión de colaboradores y plantilla laboral', 'badge', 9, true, 'system'),
('MOD_ROLES', 'Roles y Permisos', 'Perfiles de acceso y seguridad granular', 'shield_lock', 10, true, 'system'),
('MOD_USERS', 'Cuentas de Usuario', 'Credenciales, cuentas y accesos del sistema', 'manage_accounts', 11, true, 'system'),
('MOD_PAYMENTS', 'Formas de Pago', 'Métodos de pago y cuentas bancarias', 'payments', 12, true, 'system'),
('MOD_INTEGRATIONS', 'Notificaciones e Integraciones', 'Configuración de pasarelas y WhatsApp Cloud API', 'hub', 13, true, 'system'),
('MOD_MEMBERSHIPS', 'Planes y Membresías', 'Gestión de suscripciones y beneficios', 'card_membership', 14, true, 'system'),
('MOD_FAQS', 'Preguntas Frecuentes', 'Gestión de preguntas frecuentes y categorías', 'quiz', 15, true, 'system'),
('MOD_SUGGESTIONS', 'Buzón de Sugerencias', 'Gestión de sugerencias y rutas deseadas', 'inbox', 16, true, 'system'),
('MOD_LICENSE', 'Licenciamiento OmniSphere', 'Activación y validación de licencias de producto', 'key', 17, true, 'system')
ON CONFLICT ("Code") DO UPDATE SET
    "Name" = EXCLUDED."Name",
    "Description" = EXCLUDED."Description",
    "Icon" = EXCLUDED."Icon",
    "SortOrder" = EXCLUDED."SortOrder",
    "IsActive" = true;

-- Seed Permissions (Atomic Operational Permissions por cada acción del frontend)
INSERT INTO "Permissions" ("Code", "Name", "Description", "ModuleCode", "IsActive", "CreatedBy") VALUES
-- 01. Overview
('ROUTE_OVERVIEW_EXPORT', 'Exportar Métricas', 'Permite exportar resúmenes ejecutivos en Excel o PDF', 'MOD_OVERVIEW', true, 'system'),

-- 02. Reservaciones
('ROUTE_RESERVATION_VIEW', 'Consultar Reservaciones', 'Permite ver el listado y detalle de reservaciones', 'MOD_RESERVATIONS', true, 'system'),
('ROUTE_RESERVATION_CREATE', 'Crear Reservación', 'Permite registrar nuevas reservaciones en el sistema', 'MOD_RESERVATIONS', true, 'system'),
('ROUTE_RESERVATION_UPDATE', 'Editar Reservación', 'Permite modificar datos del pasajero o viaje', 'MOD_RESERVATIONS', true, 'system'),
('ROUTE_RESERVATION_STATUS', 'Cambiar Estatus', 'Permite cambiar estado de reservación (Confirmar, etc.)', 'MOD_RESERVATIONS', true, 'system'),
('ROUTE_RESERVATION_PAY', 'Procesar Pagos', 'Permite validar o registrar cobros de reservaciones', 'MOD_RESERVATIONS', true, 'system'),
('ROUTE_RESERVATION_RECEIPT', 'Ver Comprobante', 'Permite visualizar e imprimir comprobantes y boletos', 'MOD_RESERVATIONS', true, 'system'),
('ROUTE_RESERVATION_CANCEL', 'Cancelar Reservación', 'Permite anular reservaciones y liberar asientos', 'MOD_RESERVATIONS', true, 'system'),
('ROUTE_RESERVATION_DELETE', 'Eliminar Registro', 'Permite borrado lógico de reservaciones', 'MOD_RESERVATIONS', true, 'system'),

-- 03. Horarios y Salidas
('ROUTE_SCHEDULE_VIEW', 'Consultar Horarios', 'Permite ver salidas y corridas programadas', 'MOD_SCHEDULES', true, 'system'),
('ROUTE_SCHEDULE_CREATE', 'Crear Horario', 'Permite programar nuevas corridas y viajes', 'MOD_SCHEDULES', true, 'system'),
('ROUTE_SCHEDULE_UPDATE', 'Editar Horario', 'Permite modificar horas, capacidad o tarifas de viaje', 'MOD_SCHEDULES', true, 'system'),
('ROUTE_SCHEDULE_DELETE', 'Eliminar Horario', 'Permite dar de baja una corrida programada', 'MOD_SCHEDULES', true, 'system'),

-- 04. Destinos (Venues)
('ROUTE_VENUE_VIEW', 'Consultar Destinos', 'Permite ver recintos y destinos registrados', 'MOD_VENUES', true, 'system'),
('ROUTE_VENUE_CREATE', 'Registrar Destino', 'Permite dar de alta nuevos destinos y recintos', 'MOD_VENUES', true, 'system'),
('ROUTE_VENUE_UPDATE', 'Modificar Destino', 'Permite editar dirección, ciudad y nombre del destino', 'MOD_VENUES', true, 'system'),
('ROUTE_VENUE_DELETE', 'Eliminar Destino', 'Permite eliminar destinos registrados', 'MOD_VENUES', true, 'system'),

-- 05. Orígenes y Paradas (Points)
('ROUTE_POINT_VIEW', 'Consultar Paradas', 'Permite consultar puntos de abordaje y paradas', 'MOD_POINTS', true, 'system'),
('ROUTE_POINT_CREATE', 'Crear Punto de Abordaje', 'Permite dar de alta nuevas paradas y terminales', 'MOD_POINTS', true, 'system'),
('ROUTE_POINT_UPDATE', 'Editar Parada', 'Permite modificar dirección y tipo de punto de abordaje', 'MOD_POINTS', true, 'system'),
('ROUTE_POINT_DELETE', 'Eliminar Parada', 'Permite retirar paradas del catálogo', 'MOD_POINTS', true, 'system'),

-- 06. Rutas de Viaje
('ROUTE_ROUTE_VIEW', 'Consultar Rutas', 'Permite ver el catálogo de rutas e itinerarios', 'MOD_ROUTES', true, 'system'),
('ROUTE_ROUTE_CREATE', 'Trazar Ruta', 'Permite crear itinerarios y rutas de viaje', 'MOD_ROUTES', true, 'system'),
('ROUTE_ROUTE_UPDATE', 'Editar Ruta', 'Permite modificar paradas o precios de la ruta', 'MOD_ROUTES', true, 'system'),
('ROUTE_ROUTE_DELETE', 'Eliminar Ruta', 'Permite anular rutas existentes', 'MOD_ROUTES', true, 'system'),

-- 07. Eventos
('ROUTE_EVENT_VIEW', 'Consultar Eventos', 'Permite ver la cartelera de eventos programados', 'MOD_EVENTS', true, 'system'),
('ROUTE_EVENT_CREATE', 'Crear Evento', 'Permite dar de alta nuevos conciertos o festivales', 'MOD_EVENTS', true, 'system'),
('ROUTE_EVENT_UPDATE', 'Editar Evento', 'Permite actualizar datos, fecha o imagen del evento', 'MOD_EVENTS', true, 'system'),
('ROUTE_EVENT_DELETE', 'Eliminar Evento', 'Permite retirar eventos del sistema', 'MOD_EVENTS', true, 'system'),

-- 08. Configuración General
('CORE_CONFIG_UPDATE', 'Actualizar Configuración', 'Permite ajustar parámetros generales y políticas', 'MOD_SETTINGS', true, 'system'),

-- 09. Empleados y Personal
('CORE_EMPLOYEE_VIEW', 'Consultar Empleados', 'Permite ver el directorio de colaboradores y empleados', 'MOD_EMPLOYEES', true, 'system'),
('CORE_EMPLOYEE_CREATE', 'Registrar Empleado', 'Permite dar de alta nuevo personal operativo o staff', 'MOD_EMPLOYEES', true, 'system'),
('CORE_EMPLOYEE_UPDATE', 'Modificar Empleado', 'Permite editar datos laborales y de contacto', 'MOD_EMPLOYEES', true, 'system'),
('CORE_EMPLOYEE_DELETE', 'Eliminar Empleado', 'Permite dar de baja colaboradores', 'MOD_EMPLOYEES', true, 'system'),

-- 10. Roles y Permisos
('CORE_ROLE_CREATE', 'Crear Rol', 'Permite registrar nuevos perfiles de acceso', 'MOD_ROLES', true, 'system'),
('CORE_ROLE_UPDATE', 'Modificar Rol', 'Permite editar nombre y atributos del rol', 'MOD_ROLES', true, 'system'),
('CORE_ROLE_DELETE', 'Eliminar Rol', 'Permite dar de baja roles del sistema', 'MOD_ROLES', true, 'system'),
('CORE_ROLE_MANAGE', 'Administrar Roles', 'Permite crear o ajustar perfiles de seguridad', 'MOD_ROLES', true, 'system'),
('CORE_PERM_MANAGE', 'Modificar Permisos', 'Permite otorgar o revocar permisos específicos', 'MOD_ROLES', true, 'system'),

-- 11. Cuentas de Usuario
('CORE_USER_VIEW', 'Consultar Usuarios', 'Permite ver las cuentas de usuario registradas', 'MOD_USERS', true, 'system'),
('CORE_USER_CREATE', 'Crear Cuenta de Usuario', 'Permite registrar credenciales y cuentas de acceso', 'MOD_USERS', true, 'system'),
('CORE_USER_UPDATE', 'Modificar Usuario', 'Permite editar contraseñas, roles y teléfonos', 'MOD_USERS', true, 'system'),
('CORE_USER_DELETE', 'Eliminar Usuario', 'Permite dar de baja cuentas de usuario', 'MOD_USERS', true, 'system'),

-- 12. Formas de Pago
('CORE_PAYMENT_CREATE', 'Crear Método de Pago', 'Permite dar de alta nuevas formas de cobro', 'MOD_PAYMENTS', true, 'system'),
('CORE_PAYMENT_UPDATE', 'Editar Configuración de Pago', 'Permite editar comisiones y pasarelas', 'MOD_PAYMENTS', true, 'system'),
('CORE_PAYMENT_BANK_MANAGE', 'Administrar Cuentas Bancarias', 'Permite editar CLABEs, bancos y beneficiarios', 'MOD_PAYMENTS', true, 'system'),
('CORE_PAYMENT_DELETE', 'Eliminar Método de Pago', 'Permite desactivar opciones de cobro', 'MOD_PAYMENTS', true, 'system'),

-- 13. Notificaciones e Integraciones
('CORE_INTEGRATION_STRIPE', 'Configurar Stripe', 'Permite configurar API keys y webhooks de cobros', 'MOD_INTEGRATIONS', true, 'system'),
('CORE_INTEGRATION_WHATSAPP', 'Configurar WhatsApp', 'Permite editar tokens y plantillas de Meta Cloud', 'MOD_INTEGRATIONS', true, 'system'),
('CORE_INTEGRATION_TEST', 'Pruebas de Diagnóstico', 'Permite enviar mensajes y cobros de prueba', 'MOD_INTEGRATIONS', true, 'system'),
('CORE_NOTIF_STAFF', 'Notificaciones a Personal', 'Permite configurar alertas de salida a operadores', 'MOD_INTEGRATIONS', true, 'system'),

-- 14. Planes y Membresías
('CORE_MEMBERSHIP_MANAGE', 'Administrar Planes', 'Permite crear o modificar beneficios y membresías', 'MOD_MEMBERSHIPS', true, 'system'),

-- 15. Preguntas Frecuentes (FAQs)
('CORE_FAQ_CREATE', 'Crear Pregunta/Categoría', 'Permite registrar nuevas preguntas frecuentes y categorías', 'MOD_FAQS', true, 'system'),
('CORE_FAQ_UPDATE', 'Editar Pregunta/Categoría', 'Permite modificar respuestas, visibilidad y orden', 'MOD_FAQS', true, 'system'),
('CORE_FAQ_DELETE', 'Eliminar Pregunta/Categoría', 'Permite retirar preguntas frecuentes del catálogo', 'MOD_FAQS', true, 'system'),

-- 16. Buzón de Sugerencias
('CORE_FEEDBACK_VIEW', 'Consultar Buzón', 'Permite ver y filtrar comentarios y rutas sugeridas', 'MOD_SUGGESTIONS', true, 'system'),
('CORE_FEEDBACK_STATUS', 'Marcar como Atendido', 'Permite cambiar el estatus de revisión y notas', 'MOD_SUGGESTIONS', true, 'system'),
('CORE_FEEDBACK_DELETE', 'Eliminar Sugerencia', 'Permite dar de baja un comentario o sugerencia', 'MOD_SUGGESTIONS', true, 'system'),

-- 17. Licenciamiento OmniSphere
('CORE_LICENSE_MANAGE', 'Administrar Licencias', 'Permite activar claves y gestionar licencias del sistema', 'MOD_LICENSE', true, 'system')
ON CONFLICT ("Code") DO UPDATE SET
    "Name" = EXCLUDED."Name",
    "Description" = EXCLUDED."Description",
    "ModuleCode" = EXCLUDED."ModuleCode",
    "IsActive" = true;

-- Conceder todos los permisos al rol ADMIN
INSERT INTO "RolePermissions" ("RoleCode", "PermissionCode", "IsActive", "CreatedBy")
SELECT 'ADMIN', "Code", true, 'system'
FROM "Permissions"
ON CONFLICT ("RoleCode", "PermissionCode") DO UPDATE SET "IsActive" = true;

-- Conceder todos los módulos al rol ADMIN
INSERT INTO "RoleModules" ("RoleCode", "ModuleCode", "IsActive", "CreatedBy")
SELECT 'ADMIN', "Code", true, 'system'
FROM "Modules"
ON CONFLICT ("RoleCode", "ModuleCode") DO UPDATE SET "IsActive" = true;

-- Access Control & Permissions catalog seeded. No default roles created.


