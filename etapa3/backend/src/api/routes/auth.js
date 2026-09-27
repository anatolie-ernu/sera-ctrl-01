'use strict';
const router = require('express').Router();
const bcrypt = require('bcrypt');
const jwt    = require('jsonwebtoken');
const crypto = require('crypto');
const db     = require('../../db/pool');
const {auth} = require('../middleware/auth');
const sign   = (u,exp) => jwt.sign({id:u.id,username:u.username,role:u.role},process.env.JWT_SECRET,{expiresIn:exp});
const signR  = (u)     => jwt.sign({id:u.id},process.env.JWT_REFRESH_SECRET,{expiresIn:'30d'});
const hash   = (t)     => crypto.createHash('sha256').update(t).digest('hex');

router.post('/login', async (req, res) => {
    const {username,password} = req.body;
    if(!username||!password) return res.status(400).json({error:'Campuri lipsa'});
    try {
        const u = await db.queryOne('SELECT * FROM users WHERE username=$1 OR email=$1',[username]);
        if(!u||!await bcrypt.compare(password,u.password)) return res.status(401).json({error:'Credentiale incorecte'});
        const access=sign(u,'15m'), refresh=signR(u);
        await db.query('INSERT INTO refresh_tokens(user_id,token_hash,expires_at) VALUES($1,$2,NOW()+INTERVAL\'30 days\')',[u.id,hash(refresh)]);
        res.json({access_token:access,refresh_token:refresh,expires_in:900,user:{id:u.id,username:u.username,email:u.email,role:u.role}});
    } catch(e) { res.status(500).json({error:e.message}); }
});
router.post('/refresh', async (req, res) => {
    const {refresh_token} = req.body;
    if(!refresh_token) return res.status(400).json({error:'Token lipsa'});
    try {
        const dec = jwt.verify(refresh_token,process.env.JWT_REFRESH_SECRET);
        const stored = await db.queryOne('SELECT * FROM refresh_tokens WHERE token_hash=$1 AND revoked=false AND expires_at>NOW()',[hash(refresh_token)]);
        if(!stored) return res.status(401).json({error:'Token invalid'});
        const u = await db.queryOne('SELECT * FROM users WHERE id=$1',[dec.id]);
        const access=sign(u,'15m'), refresh=signR(u);
        await db.query('UPDATE refresh_tokens SET revoked=true WHERE id=$1',[stored.id]);
        await db.query('INSERT INTO refresh_tokens(user_id,token_hash,expires_at) VALUES($1,$2,NOW()+INTERVAL\'30 days\')',[u.id,hash(refresh)]);
        res.json({access_token:access,refresh_token:refresh,expires_in:900});
    } catch { res.status(401).json({error:'Token invalid'}); }
});
router.post('/logout', auth, async (req,res) => {
    const {refresh_token} = req.body;
    if(refresh_token) await db.query('UPDATE refresh_tokens SET revoked=true WHERE token_hash=$1',[hash(refresh_token)]).catch(()=>{});
    res.json({message:'Deconectat'});
});
router.get('/me', auth, async (req,res) => {
    const u=await db.queryOne('SELECT id,username,email,role,phone,notify_email,notify_sms FROM users WHERE id=$1',[req.user.id]);
    res.json({data:u});
});
module.exports = router;
