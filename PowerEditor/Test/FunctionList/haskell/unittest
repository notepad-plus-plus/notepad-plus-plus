-- Necessary:
{-# LANGUAGE DeriveDataTypeable #-}
{-# LANGUAGE GADTs #-}
{-# LANGUAGE MultiParamTypeClasses #-}
{-# LANGUAGE OverloadedStrings #-}
{-# LANGUAGE StandaloneDeriving #-}
{-# LANGUAGE TypeFamilies #-}

-- Incidental:
{-# LANGUAGE FlexibleInstances #-}
{-# LANGUAGE TypeSynonymInstances #-}

module Main where

import Control.Monad
import Data.Hashable
import Data.List
import Data.Text (Text)
import Data.Traversable (for)
import Data.Typeable
import Haxl.Core
import System.Random

import qualified Data.Text as Text

main :: IO ()
main = do
  let stateStore = stateSet UserState{} stateEmpty
  env0 <- initEnv stateStore ()
  names <- runHaxl env0 getAllUsernames
  print names

-- Data source API.

getAllUsernames :: Haxl [Name]
getAllUsernames = do
  userIds <- getAllUserIds
  for userIds $ \userId -> do
    getUsernameById userId

getAllUserIds :: Haxl [Id]
getAllUserIds = dataFetch GetAllIds

getUsernameById :: Id -> Haxl Name
getUsernameById userId = dataFetch (GetNameById userId)

-- Aliases.

type Haxl = GenHaxl () ()
type Id = Int
type Name = Text

-- Data source implementation.

data UserReq a where
  GetAllIds   :: UserReq [Id]
  GetNameById :: Id -> UserReq Name
  deriving (Typeable)

deriving instance Eq (UserReq a)
instance Hashable (UserReq a) where
   hashWithSalt s GetAllIds       = hashWithSalt s (0::Int)
   hashWithSalt s (GetNameById a) = hashWithSalt s (1::Int, a)

deriving instance Show (UserReq a)
instance ShowP UserReq where showp = show

instance StateKey UserReq where
  data State UserReq = UserState {}

instance DataSourceName UserReq where
  dataSourceName _ = "UserDataSource"

instance DataSource u UserReq where
  fetch _state _flags _userEnv = SyncFetch $ \blockedFetches -> do
    let
      allIdVars :: [ResultVar [Id]]
      allIdVars = [r | BlockedFetch GetAllIds r <- blockedFetches]

      idStrings :: [String]
      idStrings = map show ids

      ids :: [Id]
      vars :: [ResultVar Name]
      (ids, vars) = unzip
        [(userId, r) | BlockedFetch (GetNameById userId) r <- blockedFetches]

    unless (null allIdVars) $ do
      allIds <- sql "select id from ids"
      mapM_ (\r -> putSuccess r allIds) allIdVars

    unless (null ids) $ do
      names <- sql $ unwords
        [ "select name from names where"
        , intercalate " or " $ map ("id = " ++) idStrings
        , "order by find_in_set(id, '" ++ intercalate "," idStrings ++ "')"
        ]
      mapM_ (uncurry putSuccess) (zip vars names)

-- Mock SQL API.

class SQLResult a where
  mockResult :: IO a

instance SQLResult a => SQLResult [a] where
  mockResult = replicateM 10 mockResult

instance SQLResult Name where
  -- An infinite number of employees, all named Jim.
  mockResult = ("Jim" `Text.append`) . Text.pack . show <$> randomRIO (1::Int, 100)

instance SQLResult Id where
  mockResult = randomRIO (1, 100)

sql :: SQLResult a => String -> IO a
sql query = print query >> mockResult
